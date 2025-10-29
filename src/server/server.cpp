/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:39 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/29 19:25:18 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "config.hpp"
#include "socket.hpp"
#include <ctime>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
Server::Server(const ConfigData& config)
	:_configData(config){

	_listeningSockets.clear();
	initializeListeningSockets();
	_clients.clear();
}
Server::~Server(){
	shutdown();
}

void Server::initializeListeningSockets(){

	//add logic for incoming listening sockets from the config file.
	for(size_t i = 0; i < _configData.listeners.size(); i++){

		Socket listenSocket;

		listenSocket.createDefault();

		if (listenSocket.getFd() < 0) {
			throw std::runtime_error("Failed to create socket");
		}

		listenSocket.setReuseAddr(true);
		listenSocket.binding(_configData.listeners[i].second);
		if (listenSocket.getFd() < 0) {
			throw std::runtime_error("Failed to bind socket (port may be in use)");
		}

		listenSocket.listening(_configData.backlog);
		if (listenSocket.getFd() < 0) {
			throw std::runtime_error("Failed to listen on socket");
		}

		listenSocket.setNonBlocking();

		_listeningSockets.push_back(listenSocket);

		std::cout << "Susesfully added listening socket fd: "
				  << listenSocket.getFd() << " at vector position: "
				  << i << std::endl;
	}
}

void Server::handleEvent(int fd, short revents) {

	int listenFdIndex = isListeningSocket(fd);
	if (listenFdIndex >= 0) {
		if (revents & POLLIN) {
			handleListenEvent(listenFdIndex);
		}
	} else {
		// Client socket
		if (revents & POLLIN) {
			handleClientRead(fd);
		}
		if (revents & POLLOUT) {
			handleClientWrite(fd);
		}
		if (revents & POLLERR) {
			std::cerr << "ERROR: Listening socket FD " << fd
			<< " is invalid (POLLNVAL). Server socket not properly initialized." << std::endl;
			disconectClient(fd);
		}
		if (revents & POLLHUP) {
			std::cout << "Client FD " << fd << " hung up" << std::endl;
			disconectClient(fd);
		}
	}
}

void Server::handleListenEvent(int indexOfLinstenSocket){

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
		_clients[client_fd].socket.setNonBlocking();
		std::cout << "[DEBUG] Making socket FD " << client_fd << " non-blocking" << std::endl;

	}
	else if (client_fd >= 0){
		close(client_fd);
	}
}
void Server::handleClientRead(int fd){

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
		disconectClient(fd);
	}

	if(_clients[fd].state == READING_REQUEST){

		char buffer[BUFFER_SIZE];
		std::memset(buffer, 0, BUFFER_SIZE);
		int bytes = recv(fd, buffer, BUFFER_SIZE - 1, 0);
		std::cout << "[DEBUG] recv() returned " << bytes << " bytes from FD " << fd << std::endl;

		if (bytes <= 0) {
			if (bytes == 0) {
				std::cout << "[DEBUG] Client FD " << fd << " disconnected" << std::endl;
				disconectClient(fd);
			} else { // bytes < 0

				std::cout << "[DEBUG] Error on FD " << fd << ": " << strerror(errno) << std::endl;
				disconectClient(fd);
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

				//if cgi -> cgi

				Methods method = httpRequest.getMethodEnum();
				switch (method){
					case GET: handleGET(httpRequest, _clients[fd], mappedPath); break;
					case POST: handlePOST(httpRequest, _clients[fd], mappedPath); break;
					case DELETE: handleDELETE(httpRequest, _clients[fd], mappedPath); break;
				}

				_clients[fd].bytesSent = 0;
				_clients[fd].state = SENDING_RESPONSE;
				std::cout << "[DEBUG] Switched FD " << fd << " to POLLOUT mode (ready to send response)" << std::endl;
			}
		}
	}
		std::cout << "#################################\n" << std::endl;

}
void Server::handleClientWrite(int fd){

	if (_clients[fd].state == SENDING_RESPONSE){

		std::cout << "POLLOUT event on client FD " << fd << " (sending response)" << std::endl;

		// Send remaining response data
		const char* data = _clients[fd].responseData.c_str() + _clients[fd].bytesSent;
		size_t remainingLean = _clients[fd].responseData.length() - _clients[fd].bytesSent;

		int bytes_sent = send(fd, data, remainingLean, 0);
		std::cout << "send() returned " << bytes_sent << " bytes to FD " << fd << std::endl;

		if (bytes_sent > 0) {

			_clients[fd].bytesSent += bytes_sent;

			updateClientActivity(fd);


			std::cout << "Bytes setn: " << _clients[fd].bytesSent << "    ResponseData length: " << _clients[fd].responseData.length() << std::endl;

			// Check if entire response was sent
			if (_clients[fd].bytesSent == _clients[fd].responseData.length()) {

				if(_clients[fd].shouldClose){
					std::cout << "Complete response sent to FD " << fd << ". Closing connection." << std::endl;
					disconectClient(fd);
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
			disconectClient(fd);
		}
	}
}

void Server::handleGET(const HttpRequest& request, ClientInfo& client, std::string mappedPath){

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

	/*
	The GET method finds the directory/file, reads it, and uses HttpResponse methods to
	properly format the HTTP response with headers, status codes, etc.

	1. Parse the request path → /index.html, /images/logo.png (done in httpRequest)
	2. Map to file system path → /var/www/html/index.html
	3. Check if file exists and is readable
	4. Read file contents (if it exists)
	5. Use HttpResponse to build response


	//example of the get function

	void Server::handleGET(const HttpRequest& request, ClientInfo& client) {
	std::string requestedPath = request.getPath();  // "/index.html"
	std::string filePath = _documentRoot + requestedPath;  // "/var/www/html/index.html"

	HttpResponse response;

	// Try to read file
	std::ifstream file(filePath);
	if (file.is_open()) {
		// File exists - read content
		std::string content((std::istreambuf_iterator<char>(file)),
							std::istreambuf_iterator<char>());

		response.setStatusCode(200);
		response.setReasonPhrase("OK");
		response.setContentType("text/html");  // or detect from extension
		response.setBody(content);
	} else {
		// File not found
		response.setStatusCode(404);
		response.setReasonPhrase("Not Found");
		response.setContentType("text/html");
		response.setBody("<h1>404 - File Not Found</h1>");
	}

	client.responseData = response.toString();
	}
*/
}
void Server::handlePOST(const HttpRequest& request, ClientInfo& client, std::string mappedPath){

	HttpResponse response(request);

	std::cout << "[DEBUG] UploadPath: " << mappedPath << std::endl;
	PostHandler post(mappedPath);

	std::string contentType = request.getContenType();
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
/*
	DELETE Method Purpose

  The DELETE method is used to remove resources from the server.
  Unlike GET (retrieves) and POST (creates/modifies), DELETE
  specifically removes existing resources.

  DELETE Method Logic

  Looking at your GET logic pattern, here's what DELETE should
  conceptually do:

  1. Path Resolution (like GET)

  Client request: DELETE /uploads/document.pdf
  Server maps to: /var/www/uploads/document.pdf

  2. Resource Validation

  - Check if the file/resource exists
  - Verify permissions (can the client delete this?)
  - Check if it's a protected file (don't delete system files!)

  3. Security Checks

  - Validate the path (prevent ../../../etc/passwd attacks)
  - Check if deletion is allowed for this resource type
  - Verify client authorization (if implemented)

  4. Deletion Operation

  - If file exists and deletion is allowed: remove it from
  filesystem
  - If file doesn't exist: return 404 (Not Found)
  - If forbidden: return 403 (Forbidden)

  5. Response Generation (like your GET/POST pattern)

  HttpResponse response;

  if (file_deleted_successfully) {
      response.setStatusCode(204);  // No Content (successful
  deletion)
      response.setReasonPhrase("No Content");
      // No body needed for successful DELETE
  } else if (file_not_found) {
      response.setStatusCode(404);
      response.setReasonPhrase("Not Found");
  } else if (forbidden) {
      response.setStatusCode(403);
      response.setReasonPhrase("Forbidden");
  }

  Key Differences from GET/POST

  - GET: Reads and returns file content
  - POST: Accepts data to create/modify resources
  - DELETE: Removes existing resources entirely

  CGI with DELETE

  Just like your POST CGI comments, DELETE can also trigger CGI
  scripts:
  DELETE /cgi-bin/remove_user.py?id=123
  → Script processes deletion logic
  → Returns success/failure response

  The core pattern follows your existing structure: parse path →
  validate → perform operation → generate HTTP response using
  HttpResponse class.
  */
}

bool Server::validateMethod(const HttpRequest& request, const LocationConfig*& location){

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
std::string Server::mapPath(const HttpRequest& request, const LocationConfig*& matchedLocation){

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
bool Server::isPathSafe(const std::string& mappedPath, const std::string& allowedRoot){

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

void Server::disconectClient(short fd){

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
void Server::updateClientActivity(int fd){

	_clients[fd].lastActivity = time(NULL);
}
void Server::shutdown(){

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
const std::vector<Socket>& Server::getListeningSockets() const { return _listeningSockets;}
std::map<int, ClientInfo>& Server::getClients() {return _clients;}
