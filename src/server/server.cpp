/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:39 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/25 20:19:34 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "config.hpp"
#include "socket.hpp"
#include "post_handler.hpp"
#include "http_response.hpp"
#include "helpers.hpp"
#include "../cgi/cgi.hpp"

#include <iostream>
#include <ctime>
#include <fstream>
#include <errno.h>
#include <limits.h>   // for realpath
#include <unistd.h>   // for access()
#include <sys/stat.h> // for stat()


Server::Server(const ConfigData& config)
	:_configData(config){

	_listeningSockets.clear();
	initializeListeningSockets();
	_clients.clear();
}
Server::~Server(){
	shutdown();
}

void Server::initListeningSockets(){

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

	// Listening Socket
	int listenFdIndex = isListeningSocket(fd);
	if (listenFdIndex >= 0) {
		if (revents & POLLIN) {
			handleListenEvent(listenFdIndex);
		}
		return;
	}
	// CGI socket
	if (_cgi.find(fd) != _cgi.end()) {
		std::cout << "[DEBUG] Event detected on CGI FD " << fd << std::endl;
		if (revents & POLLERR) {
			std::cerr << "[DEBUG] CGI POLLERR event on FD " << fd << std::endl;
			close(fd);
		}
		else if (revents & POLLHUP) {
			std::cout << "[DEBUG] CGI POLLHUP event on FD " << fd << std::endl;
			if (revents & POLLIN) {
				handleCGIread(fd);  // final read + cleanup
			}
			close(fd);
		}
		else if (revents & POLLIN) {
			std::cout << "[DEBUG] CGI POLLIN event on FD " << fd << std::endl;
			handleCGIread(fd);
		}
		else if (revents & POLLOUT) {
			std::cout << "[DEBUG] CGI POLLOUT event on FD " << fd << std::endl;
			handleCGIwrite(fd);
		}
		return;
	}
	// Client socket
	if (_clients.find(fd) != _clients.end()) {
		if (revents & POLLERR) {
			std::cerr << "[DEBUG] Listening socket FD " << fd
			<< " is invalid (POLLNVAL). Server socket not properly initialized." << std::endl;
			disconnectClient(fd);
		}
		else if (revents & POLLHUP) {
			if (revents & POLLIN) { // half closed in case client is done sending and waiting for respose:   shutdown(fd, SHUT_WR); recv(fd, ...);
				handleClientRead(fd);
			}
			std::cout << "[DEBUG] Client FD " << fd << " hung up" << std::endl;
			disconnectClient(fd);
		}
		else if (revents & POLLIN) {
			handleClientRead(fd);
		}
		else if (revents & POLLOUT) {
			handleClientWrite(fd);
		}
		return;
	}
	std::cerr << "[WARNING] Event on unknown FD " << fd << std::endl;
}

void Server::handleListenEvent(int indexOfLinstenSocket){

	std::cout << "Event detected on listening socket FD " << _listeningSockets[indexOfLinstenSocket].getFd() << std::endl;

	short client_fd = _listeningSockets[indexOfLinstenSocket].accepting();

	if (client_fd >= 0 && _clients.size() < static_cast<size_t>(_configData.max_clients)) {

		_clients[client_fd] = ClientInfo(client_fd);
		_clients[client_fd].keepAliveTimeout = _configData.keepalive_timeout;
		_clients[client_fd].lastActivity = time(NULL);
		_clients[client_fd].maxRequests = _configData.keepalive_max_requests;
		_clients[client_fd].requestCount = 0;

		std::cout << "New connection accepted! Client FD: " << client_fd
				  << "Timeout: " << _clients[client_fd].keepAliveTimeout
				  << "Max Max Requests: " << _clients[client_fd].maxRequests
				  << std::endl;

		// Make client socket non-blocking
		_clients[client_fd].socket.setNonBlocking();
		std::cout << "Making socket FD " << client_fd << " non-blocking" << std::endl;

	}
	else if (client_fd >= 0){
		close(client_fd);
	}

}
void Server::handleClientRead(int fd){

	if (_clients[fd].requestCount >= _clients[fd].maxRequests){
		std::cout << "Max request count reached: " << fd << std::endl;

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

		char buffer[BUFFER_SIZE];
		std::memset(buffer, 0, BUFFER_SIZE);
		int bytes = recv(fd, buffer, BUFFER_SIZE - 1, 0);
		std::cout << "recv() returned " << bytes << " bytes from FD " << fd << std::endl;

		if (bytes <= 0) {
			if (bytes == 0) {
				std::cout << "Client FD " << fd << " disconnected" << std::endl;
				disconnectClient(fd);
			} else { // bytes < 0
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					// No data available - continue monitoring
					return;
				}
				std::cout << "Error on FD " << fd << ": " << strerror(errno) << std::endl;
  				disconnectClient(fd);
			}
		}
		else {

			updateClientActivity(fd);

			_clients[fd].requestData.append(buffer, bytes);
			{
			// Check if headers complete
			size_t headerEnd = _clients[fd].requestData.find("\r\n\r\n");
			if(headerEnd == std::string::npos)
				return;  // Keep receiving headers

			// Parse headers to get Content-Length
			HttpRequest tempParser;
			tempParser.partialParseRequest(_clients[fd].requestData);
			size_t contentLength = tempParser.getContentLength();

			// Check if full body received
			size_t bodyStart = headerEnd + 4;
			size_t bodyReceived = _clients[fd].requestData.length() - bodyStart;

			std::cout << "[DEBUG] Content-Length: " << contentLength
			<< ", Body received: " << bodyReceived
			<< ", Total data: " << _clients[fd].requestData.length() << std::endl;

			if(bodyReceived < contentLength)
				return;  // Keep receiving body
			}

			// Full request is received, prepare response
			HttpRequest httpRequest;
			httpRequest.parseRequest(_clients[fd].requestData);
			if(!httpRequest.getStatus()){
				HttpResponse errorResponse(httpRequest);
				errorResponse.generateResponse(400, false, "");

				_clients[fd].bytesSent = 0;
				_clients[fd].state = SENDING_RESPONSE;
				_clients[fd].shouldClose = true;
				std::cout << "Switched FD " << fd << " to POLLOUT mode (ready to send error response)" << std::endl;

			updateClientActivity(fd);

			}else{
				// Find matching location (BEFORE CGI/static)
				const LocationConfig* matchedLoc = _configData.findMatchingLocation(httpRequest.getNormalizedReqPath());
				if (!matchedLoc) {
					std::cerr << "[ERROR] No matching location found for path: " << httpRequest.getNormalizedReqPath() << std::endl;
					HttpResponse resp(httpRequest);
					resp.generateResponse(404, false , "");
					_clients[fd].responseData = resp.getResponse();
					_clients[fd].bytesSent = 0;
					_clients[fd].state = SENDING_RESPONSE;
					return;
				}


				std::string relative = httpRequest.getNormalizedReqPath().substr(matchedLoc->path.size());
				std::string mapped = joinPath(matchedLoc->root, relative);
				httpRequest.setMappedPath(mapped);

				std::cout << "[INFO] Matched location for " << httpRequest.getNormalizedReqPath()
						  << " → root=" << matchedLoc->root << std::endl;
				std::cout << "[INFO] Mapped path: " << httpRequest.getMappedPath() << std::endl;

				bool isCgi = false;
					std::string normalizedReqPath = httpRequest.getNormalizedReqPath();
					std::string cgiExt;
                    for (std::vector<std::string>::const_iterator it = matchedLoc->cgi_ext.begin(); it != matchedLoc->cgi_ext.end(); ++it) {
	                    if (normalizedReqPath.size() >= it->size() &&
		                normalizedReqPath.compare(normalizedReqPath.size() - it->size(), it->size(), *it) == 0) {
		                    isCgi = true;
		                    cgiExt = *it;
		                    break;
	                    }
                    }
				if (isCgi)
				{
					std::cout << "[CGI] Detected CGI request for path: " << httpRequest.getNormalizedReqPath() << std::endl;

					// Step 2: Build script path using location root + request path
					std::string scriptPath = httpRequest.getMappedPath();

					// If location defines explicit cgi_path (like /usr/bin/python), use it
					if (!matchedLoc->cgi_path.empty()) {
						std::cout << "[CGI] Using cgi_path(s): ";
						for (size_t i = 0; i < matchedLoc->cgi_path.size(); ++i) {
							if (i) std::cout << ", ";
							std::cout << matchedLoc->cgi_path[i];
						}
						std::cout << std::endl;
					}


					std::cout << "[CGI] Final script path: " << scriptPath << std::endl;
					Cgi *cgi = new Cgi(scriptPath, httpRequest, _clients, fd, matchedLoc, cgiExt);
					if (!cgi->start()) {
						// Failed to start CGI → send 500
						HttpResponse resp(httpRequest);
						resp.generateResponse(500, false , "");
						_clients[fd].responseData = resp.getResponse();
						_clients[fd].bytesSent = 0;
						_clients[fd].state = SENDING_RESPONSE;
						delete cgi;
						return;
					}

					// Register CGI process
					_cgi[cgi->outFd] = cgi;
					updateClientActivity(fd);
					_clients[fd].state = WAITING_CGI;
					std::cout << "[DEBUG] Spawned CGI pid = " << cgi->pid
							  << " outFd = " << cgi->outFd
							  << " for client FD = " << fd << std::endl;
					std::cout << "clients state: " << _clients[fd].state << std::endl;
					return;
				}

				Methods method = httpRequest.getMethodEnum();
				switch (method){
					case GET: handleGET(httpRequest, _clients[fd], matchedLoc); break;
					case POST: handlePOST(httpRequest, _clients[fd], matchedLoc); break;
					case DELETE: handleDELETE(httpRequest, _clients[fd], matchedLoc); break;
				}

				_clients[fd].bytesSent = 0;
				_clients[fd].state = SENDING_RESPONSE;
				std::cout << "Switched FD " << fd << " to POLLOUT mode (ready to send response)" << std::endl;
			}
		}
	}
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

			// Check if entire response was sent
			std::cout << "Bytes setn: " << _clients[fd].bytesSent
					<< "    ResponseData length: " << _clients[fd].responseData.length()
					<< std::endl;

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
void Server::handleCGIread(int fd) {

	Cgi* cgi = _cgi[fd];
	CgiReadStatus status = cgi->handleRead();

	if(status == CGI_CONTINUE) return;

	int clientFd = cgi->getClientFd();

	if (_clients.find(clientFd) != _clients.end()) {
		if(status == CGI_READY){
			HttpResponse resonse(cgi->getRequest());
			resonse.generateResponse(200, true, cgi->getResponseData());
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
	}

	std::cout << "[Server] Erasing CGI FD " << fd << " from _cgi" << std::endl;
	_cgi.erase(fd);
	delete cgi;
}
void Server::handleCGIwrite(int fd){


}

void Server::handleGET(const HttpRequest& request, ClientInfo& client, const LocationConfig* matchedLoc){
	std::cout << "matchedLocPath: " << matchedLoc->path << std::endl;
	std::string mappedPath = request.getMappedPath();
	std::cout << "mappedPath: " << mappedPath << std::endl;
	std::ifstream file(mappedPath.c_str());

	HttpResponse response(request);
	response.setPath(mappedPath);
	if (file.is_open()){

		response.generateResponse(200, false, "");
		client.responseData = response.getResponse();
	}
	else{
		std::cout << "Error: 404 path is not found" << std::endl;
		response.generateResponse(404, false, "");
		client.responseData = response.getResponse();
	}
}
void Server::handlePOST(const HttpRequest& request, ClientInfo& client, const LocationConfig* matchedLocation){

	HttpResponse response(request);
	PostHandler post(matchedLocation->upload_store + '/');

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
		response.generateResponse(415, false, "");
		client.responseData = response.getResponse();
	}
}
void Server::handleDELETE(const HttpRequest& request, ClientInfo& client, const LocationConfig* matchedLoc) {

   std::cout << "matchedLocPath: " << matchedLoc->path << std::endl;

	std::string mappedPath = request.getMappedPath();
	std::cout << "mappedPath: " << mappedPath << std::endl;
	HttpResponse response(request);
	if (!validatePath(mappedPath)){
		response.generateResponse(403, false, "");
		client.responseData = response.getResponse();
		std::cout << "Error: 403 Forbidden" << std::endl;
		return;
	}

	std::ifstream file(mappedPath.c_str());
	if (file.is_open()){
		file.close();
		if(std::remove(mappedPath.c_str()) == 0){
			response.generateResponse(204, false, "");
			client.responseData = response.getResponse();
			std::cout << "Ok" << std::endl;
		}
		else{
			response.generateResponse(403, false, "");
			client.responseData = response.getResponse();
			std::cout << "Error: 403 permission denied" << std::endl;
		}
	}
	else{
		std::cout << "Error: 404 path is not found" << std::endl;
		response.generateResponse(404, false, "");
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

int Server::isListeningSocket(int fd) const {

	for (size_t i = 0; i < _listeningSockets.size(); i++)
	{
		if (_listeningSockets[i].getFd() == fd){
			return  i;
		}
	}
	return -1;
}
void Server::disconnectClient(short fd){

	close(fd);
	_clients.erase(fd);
}
bool Server::validatePath(const std::string& path) {
	char resolved[PATH_MAX];

	// Try to resolve full absolute path
	if (!realpath(path.c_str(), resolved)) {
		std::cerr << "[SECURITY] Invalid path: " << path << std::endl;
		return false;
	}

	std::string resolvedPath(resolved);
	std::string root = _configData.root;

	// Ensure root ends with '/'
	if (!root.empty() && root[root.size() - 1] != '/')
		root += '/';

	//  Allow only files inside the configured root directory
	if (resolvedPath.find(root) == 0)
		return true;

	std::cerr << "[SECURITY] Forbidden: " << resolvedPath
			  << " is outside root " << root << std::endl;
	return false;
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
std::map<int, Cgi*>& Server::getCGI() { return _cgi;}
