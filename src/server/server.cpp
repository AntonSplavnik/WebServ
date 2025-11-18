/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:39 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/07 21:27:10 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

Server::Server(const ConfigData& config, EventLoop& controller)
	:_configData(config),
	_controller(controller){

	_listeningSockets.clear();
	_clients.clear();
	initListeningSockets();
}
Server::~Server() {
	shutdown();
}

void Server::initListeningSockets() {

	//add logic for incoming listening sockets from the config file.
	for(size_t i = 0; i < _configData.listeners.size(); i++){

		Socket listeningSocket;

		listeningSocket.createDefault();

		if (listeningSocket.getFd() < 0) {
			throw std::runtime_error("[DEBUG] Failed to create socket");
		}

		listeningSocket.setReuseAddr(true);

		listeningSocket.binding(_configData.listeners[i].second);
		if (listeningSocket.getFd() < 0) {
			throw std::runtime_error("[DEBUG] Failed to bind socket (port may be in use)");
		}

		listeningSocket.listening(_configData.backlog);
		if (listeningSocket.getFd() < 0) {
			throw std::runtime_error("[DEBUG] Failed to listen on socket");
		}

		Socket::setNonBlocking(listeningSocket.getFd());

		_listeningSockets.push_back(listeningSocket);

		std::cout << "[DEBUG] Susesfully added listening socket fd: "
				  << listeningSocket.getFd() << " at vector position: "
				  << i << std::endl;
	}
}

void Server::handleEvent(int fd, short revents) {

	// Listening FD
	int listenFdIndex = isListeningSocket(fd);
	if (listenFdIndex >= 0) {
		if (revents & POLLIN) {
			handleListenEvent(listenFdIndex);
		}
		return;
	}
	// CGI FD
	std::map<int, Cgi*>::iterator cgiIt = _cgi.find(fd);
	if (cgiIt != _cgi.end()){
		if (_clients.find(cgiIt->second->getClientFd()) != _clients.end()) {
			std::cout << "[DEBUG] Event detected on CGI FD " << fd << std::endl;
			if (revents & POLLERR) {
				std::cerr << "[DEBUG] CGI POLLERR event on FD " << fd << std::endl;
				handleCGIerror(fd);
			} else if (revents & POLLHUP) {
				std::cout << "[DEBUG] CGI POLLHUP event on FD " << fd << std::endl;
				if (revents & POLLIN) {
					handleCGIread(fd);  // final read + cleanup
				}
				terminateCGI(cgiIt->second);
			} else if (revents & POLLIN) {
				std::cout << "[DEBUG] CGI POLLIN event on FD " << fd << std::endl;
				handleCGIread(fd);
			} else if (revents & POLLOUT) {
				std::cout << "[DEBUG] CGI POLLOUT event on FD " << fd << std::endl;
				handleCGIwrite(fd);
			}
			return;
		} else {
			terminateCGI(cgiIt->second);
			return;
		}
	}
	// Client FD
	if (_clients.find(fd) != _clients.end()) {
		if (revents & POLLERR) {
			std::cerr << "[DEBUG] Listening socket FD " << fd
			<< " is invalid (POLLNVAL). Server socket not properly initialized." << std::endl;
			disconnectClient(fd);
		} else if (revents & POLLHUP) {
			if (revents & POLLIN) { // half closed in case client is done sending and waiting for respose: shutdown(fd, SHUT_WR); recv(fd, ...);
				handleClientRead(fd);
			}
			std::cout << "[DEBUG] Client FD " << fd << " hung up" << std::endl;
			disconnectClient(fd);
		} else if (revents & POLLIN) {
			handleClientRead(fd);
		} else if (revents & POLLOUT) {
			handleClientWrite(fd);
		}
		return;
	}
}

void Server::handleListenEvent(int indexOfLinstenSocket) {

	std::cout << "[DEBUG] Event detected on listening socket FD " << _listeningSockets[indexOfLinstenSocket].getFd() << std::endl;

	sockaddr_in client_addr;
	short client_fd = _listeningSockets[indexOfLinstenSocket].accepting(client_addr);

	if (client_fd >= 0 && _clients.size() < static_cast<size_t>(_configData.max_clients)) {

		_clients[client_fd] = ClientInfo(client_fd);
		_clients[client_fd].keepAliveTimeout = _configData.keepalive_timeout;
		_clients[client_fd].lastActivity = time(NULL);
		_clients[client_fd].maxRequests = _configData.keepalive_max_requests;
		_clients[client_fd].requestCount = 0;
		_clients[client_fd].ip = inet_ntoa(client_addr.sin_addr);
		_clients[client_fd].port = ntohs(client_addr.sin_port);

		std::cout << "[DEBUG] New connection accepted! Client FD: " << client_fd
				  << "Timeout: " << _clients[client_fd].keepAliveTimeout
				  << "Max Max Requests: " << _clients[client_fd].maxRequests
				  << std::endl;

		// Make client socket non-blocking
		setNonBlocking(_clients[client_fd].fd);
		std::cout << "[DEBUG] Making socket FD " << client_fd << " non-blocking" << std::endl;
	}
	else if (client_fd >= 0){
		close(client_fd);
	}
}

void Server::handleCGIerror(int fd) {

	Cgi* cgi = _cgi[fd];
	int clientFd = cgi->getClientFd();
	int inFd = cgi->getInFd();
	int outFd = cgi->getOutFd();

	if (fd == outFd) {

		std::cerr << "[WARNING] CGI OutFd error" << std::endl;

		if (_clients.find(clientFd) != _clients.end()) {
			HttpResponse response (cgi->getRequest());
			if(cgi->isFinished()){
				std::cout << "[DEBUG] POLLERR on completed CGI, data is valid" << std::endl;
				response.generateResponse(200, true, cgi->getResponseData());
				_clients[clientFd].responseData = response.getResponse();
			} else {
				std::cout << "[DEBUG] POLLERR on incompleted CGI, data is currupted" << std::endl;
				response.generateResponse(500);
				_clients[clientFd].responseData = response.getResponse();
			}
			_clients[clientFd].bytesSent = 0;
			_clients[clientFd].state = SENDING_RESPONSE;
		}

		if(cgi->getPid() > 0){
			cgi->terminate();
		}
		cgi->cleanup();

		if(inFd >= 0) _cgi.erase(inFd);
		if(outFd >= 0) _cgi.erase(outFd);

		delete cgi;
	} else if (fd == inFd) {

		std::cerr << "[WARNING] CGI inFd error" << std::endl;

		if(cgi->getRequest().getBody().size() != cgi->getBytesWrittenToCgi()) {

			cgi->terminate();

			if (_clients.find(clientFd) != _clients.end()){
				HttpResponse resp(cgi->getRequest());
				resp.generateResponse(500, false, "");
				_clients[clientFd].responseData = resp.getResponse();
				_clients[clientFd].bytesSent = 0;
				_clients[clientFd].state = SENDING_RESPONSE;
			}

			cgi->cleanup();

			if(inFd >= 0) _cgi.erase(inFd);
			if(outFd >= 0) _cgi.erase(outFd);

			delete cgi;
		} else {

			cgi->closeInFd();
			_cgi.erase(inFd);
		}
	}
}
void Server::handleCGIread(int fd) {

	Cgi* cgi = _cgi[fd];
	CgiStatus status = cgi->handleReadFromCGI();

	if(status == CGI_CONTINUE) return;

	int clientFd = cgi->getClientFd();

	if(status == CGI_READY){
		HttpResponse response(cgi->getRequest());
		response.generateResponse(200, true, cgi->getResponseData());
		_clients[clientFd].responseData = response.getResponse();
		std::cout << "[DEBUG] Switched client FD " << clientFd
					<< " to POLLOUT mode after CGI complete" << std::endl;
	}
	else if(status == CGI_ERROR){
		HttpResponse resonse(cgi->getRequest());
		resonse.generateResponse(500);
		std::cout << "[DEBUG] Switched client FD " << clientFd
					<< " to POLLOUT mode after CGI complete" << std::endl;
	}
	_clients[clientFd].bytesSent = 0;
	_clients[clientFd].state = SENDING_RESPONSE;

	std::cout << "[Server] Erasing CGI FD " << fd << " from _cgi" << std::endl;

	int inFd = cgi->getInFd();
	int outFd = cgi->getOutFd();
	if (inFd >= 0) _cgi.erase(inFd);
	if (outFd >= 0) _cgi.erase(outFd);
	delete cgi;
}
void Server::handleCGIwrite(int fd) {

	Cgi* cgi = _cgi[fd];
	CgiStatus status = cgi->handleWriteToCGI();

	if(status == CGI_CONTINUE) return;

	if(status == CGI_READY){
		int inFd = cgi->getInFd();
		cgi->closeInFd();
		if (inFd >= 0) _cgi.erase(inFd);
	}
}

void Server::handleClientRead(int fd) {

	std::cout << "\n#######  HANDLE CLIENT READ DATA #######" << std::endl;

	if (_clients[fd].requestCount >= _clients[fd].maxRequests){
		std::cout << "[DEBUG] Max request count reached: " << fd << std::endl;

		/*
			1. Graceful Closure: You should finish sending the current
			response before disconnecting
			2. Signal to Client: Send Connection: close header in the
			response before closing
			3. Not an Error: This is normal behavior, not an error condition
		*/
		disconnectClient(fd);
	}

	if(_clients[fd].state == READING_REQUEST){

		char buffer[BUFFER_SIZE_32];
		std::memset(buffer, 0, BUFFER_SIZE_32);
		int bytes = recv(fd, buffer, BUFFER_SIZE_32 - 1, 0);
		std::cout << "[DEBUG] recv() returned " << bytes << " bytes from FD " << fd << std::endl;

		if (bytes <= 0) {
			if (bytes == 0) {
				std::cout << "[DEBUG] Client FD " << fd << " disconnected" << std::endl;
				disconnectClient(fd);
			} else { // bytes < 0

				std::cout << "[DEBUG] Error on FD " << fd << ": " << strerror(errno) << std::endl;
				disconnectClient(fd);
			}
		}
		else {

			updateClientActivity(fd);
			{
				_clients[fd].requestData.append(buffer, bytes);

				// Check if headers complete
				size_t headerEnd = _clients[fd].requestData.find("\r\n\r\n");
				if(headerEnd == std::string::npos)
					return;  // Keep receiving headers

				// Parse headers to get Content-Length
				HttpRequest tempParser;
				tempParser.ParsePartialRequest(_clients[fd].requestData);
				size_t contentLength = tempParser.getContentLength();

				// Check if full body received
				size_t bodyStart = headerEnd + 4;
				size_t bodyReceived = _clients[fd].requestData.length() - bodyStart;

				std::cout << "[DEBUG] Content-Length: " << contentLength << ", Body received: " << bodyReceived << ", Total data: " << _clients[fd].requestData.length() << std::endl;

				if(bodyReceived < contentLength)
					return;  // Keep receiving body
			}

			/// <=================================== REQUEST FULLY RECEIVED receivee

			// Full request is received, prepare response
			HttpRequest httpRequest;
			httpRequest.parseRequest(_clients[fd].requestData);
			if(!httpRequest.getStatus()){
				HttpResponse errorResponse(httpRequest);
				errorResponse.generateResponse(400);
				_clients[fd].responseData = errorResponse.getResponse();
				_clients[fd].bytesSent = 0;
				_clients[fd].state = SENDING_RESPONSE;
				_clients[fd].shouldClose = true;
				std::cout << "[DEBUG] Switched FD " << fd << " to POLLOUT mode (ready to send error response)" << std::endl;
				return;
			}else{
				updateClientActivity(fd);

			//// <=================================== REQUEST PARSED

			HttpResponse response(httpRequest);
				std::cout << "\n#######  PATH MATCHING/VALIDATIONr #######" << std::endl;
				const LocationConfig* matchedLocation = _configData.findMatchingLocation(httpRequest.getPath());
				if(!matchedLocation){
					std::cout << "[DEBUG] No matched location in config file" << std::endl;
					response.generateResponse(404);
					_clients[fd].responseData = response.getResponse();
					_clients[fd].bytesSent = 0;
					_clients[fd].state = SENDING_RESPONSE;
					std::cout << "[DEBUG] Switched FD " << fd << " to POLLOUT mode (ready to send error response)" << std::endl;
					return;
				}
				if(!validateMethod(httpRequest, matchedLocation)) {
					std::cout << "[DEBUG] Path validation failed (method not allowed or missing root)" << std::endl;
					response.generateResponse(403);
					_clients[fd].responseData = response.getResponse();
					_clients[fd].bytesSent = 0;
					_clients[fd].state = SENDING_RESPONSE;
					std::cout << "[DEBUG] Switched FD " << fd << " to POLLOUT mode (ready to send error response)" << std::endl;
					return;
				}

				std::string mappedPath = mapPath(httpRequest, matchedLocation);
				if(!isPathSafe(mappedPath, matchedLocation->root)) {
					response.generateResponse(403);
					_clients[fd].responseData = response.getResponse();
					_clients[fd].bytesSent = 0;
					_clients[fd].state = SENDING_RESPONSE;
					std::cout << "[DEBUG] Switched FD " << fd << " to POLLOUT mode (ready to send error response)" << std::endl;
					return;
				}
				std::cout << "#################################\n" << std::endl;

				//// <=================================== REQUEST IS MAPPED AND VALIDATED
				//if cgi -> cgi
				if (!isCGIrequest()) {
					std::cout << "[DEBUG] Failed to start CGI" << std::endl;
					HttpResponse resp(httpRequest);
					resp.generateResponse(500);
					_clients[fd].responseData = resp.getResponse();
					_clients[fd].bytesSent = 0;
					_clients[fd].state = SENDING_RESPONSE;
					return;
				} else {
					handleCGI();
					updateClientActivity(fd);
					return;
				}

				//// <=================================== REQUEST ROUTED TO CGI

				Methods method = httpRequest.getMethodEnum();
				switch (method){
					case GET: handleGET(httpRequest, _clients[fd], mappedPath); break;
					case POST: handlePOST(httpRequest, _clients[fd], mappedPath); break;
					case DELETE: handleDELETE(httpRequest, _clients[fd], mappedPath); break;
				}

				//// <=================================== REQUEST ROUTED TO STATIC METHOD

				_clients[fd].bytesSent = 0;
				_clients[fd].state = SENDING_RESPONSE;
				std::cout << "[DEBUG] Switched FD " << fd << " to POLLOUT mode (ready to send response)" << std::endl;

				//// <=================================== STATIC METHOD RESPONSE PREP FINISHED
			}
		}
	}
	std::cout << "#################################\n" << std::endl;
}
void Server::handleClientWrite(int fd) {

	if (_clients[fd].state == SENDING_RESPONSE){

		std::cout << "POLLOUT event on client FD " << fd << " (sending response)" << std::endl;

		// Send remaining response data
		const char* data = _clients[fd].responseData.c_str() + _clients[fd].bytesSent;
		size_t remainingLean = _clients[fd].responseData.length() - _clients[fd].bytesSent;
		size_t bytesToWrite = std::min(remainingLean, static_cast<size_t>(BUFFER_SIZE_32));

		int bytes_sent = send(fd, data, bytesToWrite, 0);
		std::cout << "send() returned " << bytes_sent << " bytes to FD " << fd << std::endl;

		if (bytes_sent > 0) {

			_clients[fd].bytesSent += bytes_sent;

			updateClientActivity(fd);


			std::cout << "Bytes setn: " << _clients[fd].bytesSent << "    ResponseData length: " << _clients[fd].responseData.length() << std::endl;

			// Check if entire response was sent
			if (_clients[fd].bytesSent == _clients[fd].responseData.length()) {

				if(_clients[fd].shouldClose){
					std::cout << "Complete response sent to FD " << fd << ". Closing connection." << std::endl;
					disconnectClient(fd);
					return;
				}

				// Reset client state for next request
				_clients[fd].state = READING_REQUEST;
				_clients[fd].bytesSent = 0;
				_clients[fd].responseData.clear();
				_clients[fd].requestData.clear();
				updateClientActivity(fd);

			} else {
				std::cout << "Partial send: " << _clients[fd].bytesSent << "/" << _clients[fd].responseData.length() << " bytes sent" << std::endl;
			}
		} else {

			// Send failed, close connection
			std::cout << "Send failed for FD " << fd << ". Closing connection." << std::endl;
			disconnectClient(fd);
		}
	}
}

void Server::handleCGI() {

	Cgi* cgi = new Cgi(_controller, httpRequest, _clients[fd], mappedPath, matchedLoc, cgiExt);
	// Register CGI process
	int inFd = cgi->getInFd();
	int outFd = cgi->getOutFd();
	if (inFd >= 0) _cgi[inFd] = cgi;
	_cgi[outFd] = cgi;
	_clients[fd].state = WAITING_CGI;

	std::cout << "[DEBUG] Spawned CGI pid = " << cgi->getPid()
			<< " inFd = " << inFd
			<< " outFd = " << outFd
			<< " for client FD = " << fd << std::endl;
	std::cout << "clients state: " << _clients[fd].state << std::endl;
}
void Server::handleGET(const HttpRequest& request, ClientInfo& client, std::string mappedPath) {

	std::ifstream file(mappedPath.c_str());

	HttpResponse response(request);
	response.setPath(mappedPath);
	if (file.is_open()){

		response.generateResponse(200);
		client.responseData = response.getResponse();
	}
	else{
		std::cout << "Error: 404 path is not found" << std::endl;
		response.generateResponse(404);
		client.responseData = response.getResponse();
	}
}
void Server::handlePOST(const HttpRequest& request, ClientInfo& client, std::string mappedPath) {

	HttpResponse response(request);

	std::cout << "[DEBUG] UploadPath: " << mappedPath << std::endl;
	PostHandler post(mappedPath);

	std::string contentType = request.getContentType();
	std::cout << "[DEBUG] POST Content-Type: '" << contentType << "'" << std::endl;
	std::cout << "[DEBUG] Request valid: " << (request.getStatus() ? "true" : "false") << std::endl;

	if (contentType.find("multipart/form-data") != std::string::npos) {
		post.handleMultipart(request, client);
	}
	else if (post.isSupportedContentType(contentType)) {
		post.handleFile(request, client, contentType);
	}
	else {
		std::cout << "[DEBUG] Unsupported Content-Type: " << contentType << std::endl;
		response.generateResponse(415);
		client.responseData = response.getResponse();
	}
}
void Server::handleDELETE(const HttpRequest& request, ClientInfo& client, std::string mappedPath) {

	HttpResponse response(request);
	std::ifstream file(mappedPath.c_str());
	if (file.is_open()){
		file.close();
		if(std::remove(mappedPath.c_str()) == 0){
			response.generateResponse(204);
			client.responseData = response.getResponse();
			std::cout << "[DEBUG] Succes: 204 file deleted" << std::endl;
		}
		else{
			response.generateResponse(403);
			client.responseData = response.getResponse();
			std::cout << "[DEBUG] Error: 403 permission denied" << std::endl;
		}
	}
	else{
		std::cout << "Error: 404 path is not found" << std::endl;
		response.generateResponse(404);
		client.responseData = response.getResponse();
	}

}

bool Server::validateMethod(const HttpRequest& request, const LocationConfig*& location) {

	bool methodAllowed = false;
	for (size_t i = 0; i < location->allow_methods.size(); ++i) {
		if (location->allow_methods[i] == request.getMethod()) {
			methodAllowed = true;
			break;
		}
	}

	if (!methodAllowed) {
		std::cout << "[DEBUG] Method " << request.getMethod() << " not allowed for this location" << std::endl;
		return false;
	}

	return true;
}
std::string Server::mapPath(const HttpRequest& request, const LocationConfig*& matchedLocation) {

	std::string locationRoot = matchedLocation->root;
	std::string locationPath = matchedLocation->path;
	std::string requestPath = request.getPath();
	std::string relativePath;

	if(requestPath.compare(0, locationPath.length(), locationPath) == 0)
		relativePath = requestPath.substr(locationPath.length());
	else{
		std::cerr << "[ERROR] mapPath called with non-matching paths!" << std::endl;
		relativePath = requestPath;  // Fallback
	}

	if(!locationRoot.empty() && locationRoot[locationRoot.length() - 1 ] == '/'
		&& !relativePath.empty() && relativePath[0] == '/')
		relativePath = relativePath.substr(1);

	std::cout << "[DEBUG] LocationRoot: " << locationRoot << std::endl;
	std::cout << "[DEBUG] LocationPath: " << locationPath << std::endl;
	std::cout << "[DEBUG] RequestPath : " << requestPath << std::endl;
	std::cout << "[DEBUG] RelativePath : " << relativePath << std::endl;
	std::cout << "[DEBUG] MappedPath : " << locationRoot + relativePath << std::endl;
	return locationRoot + relativePath;
}
bool Server::isPathSafe(const std::string& mappedPath, const std::string& allowedRoot) {

	if(mappedPath.find("../") != std::string::npos || mappedPath.find("/..") != std::string::npos) {
		std::cout << "[SECURITY] Path traversal attempt detected: " << mappedPath << std::endl;
		return false;
	}

	if(mappedPath.find('\0') != std::string::npos){
		std::cout << "[SECURITY] Null byte injection detected" << std::endl;
		return false;
	}

	// Canonical path check (strongest defense)
	char resolvedPath[PATH_MAX];
	char resolvedRoot[PATH_MAX];

	// Resolve the mapped path to canonical form
	// Note: realpath() returns NULL if file doesn't exist yet (important for POST/upload)
	if (realpath(mappedPath.c_str(), resolvedPath) == NULL) {

		// File doesn't exist - for POST this is expected
		// Validate the parent directory instead
		std::string parentDir = mappedPath.substr(0, mappedPath.find_last_of('/'));
		if (parentDir.empty()) parentDir = ".";

		if (realpath(parentDir.c_str(), resolvedPath) == NULL) {
			std::cout << "[SECURITY] Invalid path or parent directory: " << mappedPath << std::endl;
			return false;
		}
		// Add back the filename for comparison
		std::string filename = mappedPath.substr(mappedPath.find_last_of('/'));
		std::string fullPath = std::string(resolvedPath) + filename;
		strncpy(resolvedPath, fullPath.c_str(), PATH_MAX - 1);
		resolvedPath[PATH_MAX - 1] = '\0';
	}

	// Resolve the allowed root
	if (realpath(allowedRoot.c_str(), resolvedRoot) == NULL) {
		std::cout << "[SECURITY] Invalid allowed root: " << allowedRoot << std::endl;
		return false;
	}

	// Check if resolved path starts with resolved root
	std::string canonicalPath(resolvedPath);
	std::string canonicalRoot(resolvedRoot);

	if (canonicalPath.compare(0, canonicalRoot.length(), canonicalRoot) != 0) {
		std::cout << "[SECURITY] Path escape attempt!" << std::endl;
		std::cout << "[SECURITY] Requested: " << mappedPath << std::endl;
		std::cout << "[SECURITY] Resolved:  " << canonicalPath << std::endl;
		std::cout << "[SECURITY] Root:      " << canonicalRoot << std::endl;
		return false;
	}
	return true;
}

void Server::disconnectClient(short fd) {
	close(fd);
	_clients.erase(fd);
}
int Server::isListeningSocket(int fd) const {
	for (size_t i = 0; i < _listeningSockets.size(); i++)
	{
		if (_listeningSockets[i].getFd() == fd){
			return  i;
		}
	}
	return -1;
}
void Server::updateClientActivity(int fd) {

	_clients[fd].lastActivity = time(NULL);
}
void Server::shutdown() {

	//Close all of listening sockets
	for (size_t i = 0; i < _listeningSockets.size(); i++)
	{
		std::cout << "Closing listening socket FD: " << _listeningSockets[i].getFd() << std::endl;
		_listeningSockets[i].closing(_listeningSockets[i].getFd());
	}

	// Close all client connections
	for (std::map<int, ClientInfo>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		close(it->first);
	}

	_clients.clear();

	std::cout << "Server " << _configData.server_names[0] <<  " stopped" << std::endl;
}

void Server::terminateCGI(Cgi* cgi) {

	cgi->terminate();
	cgi->cleanup();
	int inFd = cgi->getInFd();
	int outFd = cgi->getOutFd();
	if (inFd >= 0) _cgi.erase(inFd);
	if (outFd >= 0) _cgi.erase(outFd);
	delete cgi;
}
void Server::handleCGItimeout(Cgi* cgi) {

	int clientFd = cgi->getClientFd();
	//  Prepare 504 Gateway Timeout response
	HttpResponse resp(cgi->getRequest());
	resp.generateResponse(504, false, "");
	if (_clients.find(clientFd) != _clients.end()) {
		_clients[clientFd].responseData = resp.getResponse();
		_clients[clientFd].state = SENDING_RESPONSE;
	}
	std::cout << "[CGI TIMEOUT] Response 504 sent to client FD = " << clientFd << std::endl;
	terminateCGI(cgi);
}

std::string Server::resolveErrorPage(int statusCode, const LocationConfig* location) const {

	if (location && location->error_pages.count(statusCode)) {
		std::string path = location->error_pages.find(statusCode)->second;
		return location->root + path;
	}

	// Global server config
	if (_configData.error_pages.count(statusCode)) {
		std::string path = _configData.error_pages.find(statusCode)->second;
		return _configData.root + path;
	}

	return "";
}

const std::vector<Socket>& Server::getListeningSockets() const { return _listeningSockets;}
std::map<int, ClientInfo>& Server::getClients() {return _clients;}
