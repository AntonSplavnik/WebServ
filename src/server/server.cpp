/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:39 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/15 18:14:30 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "config.hpp"
#include "socket.hpp"
#include <ctime>
#include <fstream>

/* - initialize() - Create and configure the listening socket using your
   Socket class
  - start() - Set up poll array, add listening socket to poll
  - run() - Main event loop with poll()
  - stop() - Cleanup */
Server::Server(const ConfigData& config)
	:_configData(config){

	_listeningSockets.clear();
	initializeListeningSockets();
	_clients.clear();
}
Server::~Server(){
	shutdown();
}
/**
 * Initialisatin of listening sockets
 */
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
void Server::handlePOLLERR(int fd){

	std::cerr << "ERROR: Listening socket FD " << fd
			<< " is invalid (POLLNVAL). Server socket not properly initialized." << std::endl;
}
void Server::handlePOLLHUP(int fd){

	std::cout << "Client FD " << fd << " hung up" << std::endl;
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
		disconectClient(fd);
	}

	if(_clients[fd].state == READING_REQUEST){

		char buffer[BUFFER_SIZE];
		std::memset(buffer, 0, BUFFER_SIZE);
		int bytes = recv(fd, buffer, BUFFER_SIZE - 1, 0);
		std::cout << "recv() returned " << bytes << " bytes from FD " << fd << std::endl;

		if (bytes <= 0) {
			if (bytes == 0) {
				std::cout << "Client FD " << fd << " disconnected" << std::endl;
				disconectClient(fd);
			} else { // bytes < 0
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					// No data available - continue monitoring
					return;
				}
				std::cout << "Error on FD " << fd << ": " << strerror(errno) << std::endl;
  				disconectClient(fd);
			}
		}
		else {

			updateClientActivity(fd);
			
			_clients[fd].requestData.append(buffer, bytes);

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


			// Full request is received, prepare response
			HttpRequest httpRequest;
			httpRequest.parseRequest(_clients[fd].requestData);
			if(!httpRequest.getStatus()){
				HttpResponse errorResponse(httpRequest);
				errorResponse.generateResponse(400);

				_clients[fd].bytesSent = 0;
				_clients[fd].state = SENDING_RESPONSE;
				_clients[fd].shouldClose = true;
				std::cout << "Switched FD " << fd << " to POLLOUT mode (ready to send error response)" << std::endl;

			updateClientActivity(fd);

			}else{
				Methods method = httpRequest.getMethodEnum();
				switch (method){
					case GET: handleGET(httpRequest, _clients[fd]); break;
					case POST: handlePOST(httpRequest, _clients[fd]); break;
					case DELETE: handleDELETE(httpRequest, _clients[fd]); break;
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
					disconectClient(fd);
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
			disconectClient(fd);
		}
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
			handlePOLLERR(fd);
			disconectClient(fd);
		}
		if (revents & POLLHUP) {
			handlePOLLHUP(fd);
			disconectClient(fd);
		}
	}
}
void Server::disconectClient(short fd){

	close(fd);
	_clients.erase(fd);
}
std::string Server::mapPath(const HttpRequest& request){

	std::string localPath = _configData.root;
	std::string requestPath = request.getPath();
	std::cout << "[DEBUG] RequestPath: " << requestPath << std::endl;

	return localPath + requestPath;
}
void Server::handleGET(const HttpRequest& request, ClientInfo& client){

	std::string mappedPath = mapPath(request);
	std::cout << "mappedPath: " << mappedPath << std::endl;
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
/*
	The POST method receives data from the client, processes it, and uses HttpResponse methods to
	send back a result.

	1. Extract data from request body → form data, JSON, file uploads
	2. Process the data → save to database, execute CGI script, handle form submission
	3. Generate appropriate response → success/failure message
	4. Use HttpResponse to build response

	  Unlike GET which reads files from server, POST accepts data from client:

	- Form submissions → username=john&password=123
	- File uploads → image/document data in request body
	- API calls → JSON data for creating/updating resources
	- CGI execution → pass data to scripts for processing


	GET Method Can Handle:

	1. Static Files:
	- /index.html → read file from disk
	- /images/logo.png → serve static content

	2. Dynamic Content (CGI):
	- /cgi-bin/search.py?q=webserv → execute script with query parameters
	- /api/users?limit=10 → database query via CGI
	- /weather.php?city=paris → dynamic page generation

	3. Server-side Processing:
	- Database queries (read-only)
	- API calls to external services
	- Template rendering

	Key Differences:

	GET with CGI:
	- Parameters in URL query string: ?name=value&foo=bar
	- No request body data
	- Should be idempotent (safe to repeat)

	POST with CGI:
	- Data in request body: form data, JSON, files
	- Can modify server state (create/update/delete)
	- Not idempotent

	Example GET with CGI:

	void Server::handleGET(const HttpRequest& request, ClientInfo& client) {
		std::string path = request.getPath();  // "/cgi-bin/search.py"

		if (path.find("/cgi-bin/") == 0) {
			// Execute CGI script with query parameters
			executeCGI(path, request.getQuery(), "", client);  // No body for GET
		} else {
			// Serve static file
			serveStaticFile(path, client);
		}
	}
*/
void Server::handlePOST(const HttpRequest& request, ClientInfo& client){

	HttpResponse response(request);
	const LocationConfig* matchedLocation = _configData.findMatchingLocation(request.getPath());
	if(!matchedLocation){
		std::cout << "[DEBUG] No matching path found. matchedLocation = NULL" << std::endl;
		response.generateResponse(403); // Forbidden no upload location configured
		client.responseData = response.getResponse();
		return;
	}

	std::cout << "[DEBUG] UploadPath: " << matchedLocation->upload_store << std::endl;
	PostHandler post(matchedLocation->upload_store + '/');

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
/*
	CGI for GET and POST

	Without CGI (Static):
	- GET /index.html → Server reads /var/www/index.html file and sends it
	- POST /contact.html → Server processes form data itself and sends response

	With CGI (Dynamic):
	- GET /cgi-bin/weather.py?city=paris → Server executes Python script, script generates HTML
	response
	- POST /cgi-bin/contact.py → Server passes form data to Python script, script processes it and
	generates response

	CGI Role:

	CGI = External program that generates web content

	Instead of serving static files, the server:
	1. Executes a program (Python, PHP, C++, etc.)
	2. Passes request data to the program
	3. Captures program's output
	4. Sends that output as HTTP response

	Examples:

	GET with CGI:
	Client: GET /cgi-bin/time.py
	Server: executes time.py script
	Script: prints "Current time: 2:30 PM"
	Server: sends that as HTTP response

	POST with CGI:
	Client: POST /cgi-bin/save.py (with form data)
	Server: executes save.py, passes form data to it
	Script: processes data, prints "Data saved successfully"
	Server: sends that as HTTP response

	CGI turns your server into a platform that can run any program to generate web pages
	dynamically.
*/
bool Server::validatePath(std::string path){
    const LocationConfig* location = _configData.findMatchingLocation("/uploads");
	if (!location || location->upload_store.empty()) {
		std::cout << "[DEBUG] No upload location found" << std::endl;
		return false;
    }
	//security check
	if (path.substr(0, location->upload_store.length()) != location->upload_store) {
		std::cout << "[DEBUG] Path is not in upload directory" << std::endl;
		return false;
	}

	bool deleteAllowed = false;
	for (size_t i = 0; i < location->allow_methods.size(); ++i) {
		if (location->allow_methods[i] == "DELETE") {
			deleteAllowed = true;
			break;
		}
	}
	if (!deleteAllowed) {
		std::cout << "[DEBUG] DELETE method not allowed for this location" << std::endl;
		return false;
    }
	return true;
}
/**
 * This function only handles files, but not directories.
 */
 void Server::handleDELETE(const HttpRequest& request, ClientInfo& client) {

	std::string mappedPath = mapPath(request);
	HttpResponse response(request);
	if (!validatePath(mappedPath)){
		response.generateResponse(403);
		client.responseData = response.getResponse();
		std::cout << "Error: 403 Forbidden" << std::endl;
		return;
	}

	std::ifstream file(mappedPath.c_str());
	if (file.is_open()){
		file.close();
		if(std::remove(mappedPath.c_str()) == 0){
			response.generateResponse(204);
			client.responseData = response.getResponse();
			std::cout << "Succes: 204 file deleted" << std::endl;
		}
		else{
			response.generateResponse(403);
			client.responseData = response.getResponse();
			std::cout << "Error: 403 permission denied" << std::endl;
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
