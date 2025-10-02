/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:39 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/02 14:34:01 by antonsplavn      ###   ########.fr       */
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

Server::Server()
	:_host("0.0.0.0"),
	_port(8080),
	_running(false),
	_maxClients(1000){

	_pollFds.clear();
	_clients.clear();
	// _pollFds.reserve(_maxClients + 1); (optimisation)
}

Server::~Server(){
	if (_running)
		stop();
}

void Server::stop(){
	_running = false;

	if(_serverSocket.getFd() >= 0){
		std::cout << "Closing listening soclet FD" << _serverSocket.getFd() << std::endl;
		_serverSocket.closing(_serverSocket.getFd());
	}

	// Close all client connections
	for (std::map<int, ClientInfo>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		close(it->first);
	}
	_clients.clear();
	_pollFds.clear();

	std::cout << "Server stopped" << std::endl;
}

void Server::initialize(){

	_serverSocket.createDefault();
	_serverSocket.setReuseAddr(true);
	_serverSocket.binding(_port);
	_serverSocket.listening();
	_serverSocket.setNonBlocking();
}

void Server::start(){

	struct pollfd serverPollFd;
	serverPollFd.fd = _serverSocket.getFd();
	serverPollFd.events = POLLIN;
	serverPollFd.revents = 0;

	_pollFds.push_back(serverPollFd);

	std::cout << "Added listening socket FD " <<
	_serverSocket.getFd() << " to poll vector at index 0" << std::endl;
}

void Server::run(){
/*
1. Set _running = true
2. Main while loop with poll()
3. Handle poll events
4. Check listening socket for new connections
5. Check client sockets for data
 */
	_running = true;

	while(_running){

		int ret = poll(_pollFds.data(), _pollFds.size(), -1);
		if (ret < 0) {
			std::cerr << "Poll failed.\n";
				break;
		}

		std::cout << "poll() returned " << ret << " (number of FDs with events)" << std::endl;
		handlePollEvents();
		checkClientTimeouts();
	}
}

/*
Then implement:
- handlePollEvents() - Check each fd in _pollFds
- handleServerSocket() - Accept new connections
- handleClientSocket() - Process client data
*/
void Server::handlePollEvents(){

	for (size_t i = 0; i < _pollFds.size(); i++){

		if(i == 0)
			handleServerSocket(i);
		else
			handleClientSocket(_pollFds[i].fd, _pollFds[i].revents);
	}
}


void Server::handleServerSocket(size_t index){

	if (_pollFds.data()->revents & POLLIN){
		std::cout << "Event detected on listening socket FD " << _pollFds.data()->fd << std::endl;

		//Accept the new connection
		short client_fd = _serverSocket.accepting();

		if (client_fd >= 0 && _clients.size() < _maxClients) {

			_clients[client_fd] = ClientInfo(client_fd);
			_clients[client_fd].keepAliveTimeout = 15;
			_clients[client_fd].lastActivity = time(NULL);
			_clients[client_fd].maxRequests = 100;
			_clients[client_fd].requestCount = 0;
			std::cout << "New connection accepted! Client FD: " << client_fd
					  << "Timeout: " << _clients[client_fd].keepAliveTimeout
					  << "Max Max Requests: " << _clients[client_fd].maxRequests
					  << std::endl;


			// Make client socket non-blocking
			_clients[client_fd].socket.setNonBlocking();
			std::cout << "Making socket FD " << client_fd << " non-blocking" << std::endl;

			// Add client socket to poll array
			struct pollfd clientPollFd;
			clientPollFd.fd = client_fd;
			clientPollFd.events = POLLIN;
			clientPollFd.revents = 0;
			_pollFds.push_back(clientPollFd);

			std::cout << "Added client socket FD " << client_fd << " to poll vector at index " << index << std::endl;
			std::cout << "New client connected. Total FDs: " << _pollFds.size() << std::endl;
		}
		else if (client_fd >= 0){
			close(client_fd);
		}
	}
}

Methods stringToMethod(const std::string& method) {
	if (method == "GET") return GET;
	if (method == "POST") return POST;
	if (method == "DELETE") return DELETE;
	throw std::invalid_argument("Unknown method");
}

void Server::handleClientSocket(short fd, short revents){

	if (_clients[fd].requestCount >= _clients[fd].maxRequests){
		std::cout << "Max request count reachead: " << fd << std::endl;
		disconectClient(fd);
	}

	if(revents & POLLIN && _clients[fd].state == READING_REQUEST){

		char buffer[BUFFER_SIZE];
		std::memset(buffer, 0, BUFFER_SIZE);
		int bytes = recv(fd, buffer, BUFFER_SIZE - 1, 0);

		if (bytes <= 0) {
			// Client disconnected or error
			std::cout << "Client FD " << fd << " disconnected" << std::endl;
			disconectClient(fd);
		}
		else {

			updateClientActivity(fd);

			std::cout << "recv() returned " << bytes << " bytes from FD " << fd << std::endl;

			_clients[fd].requestData += buffer;
			if(_clients[fd].requestData.find("\r\n\r\n") == std::string::npos)
				return;
			std::cout << "Request is received from FD " << fd << ":\n" << _clients[fd].requestData << std::endl;

			/*
			  updateClientActivity(fd);  // Line 191 - keep this

			_clients[fd].requestData += buffer;

			// Check if headers complete
			size_t headerEnd = _clients[fd].requestData.find("\r\n\r\n");
			if(headerEnd == std::string::npos)
				return;  // Keep receiving headers

			// Parse headers to get Content-Length
			HttpRequest tempParser;
			tempParser.parseHeaders(_clients[fd].requestData);  // Parse headers only
			int contentLength = tempParser.getContentLength();

			// Check if full body received
			size_t bodyStart = headerEnd + 4;
			size_t bodyReceived = _clients[fd].requestData.length() - bodyStart;
			if(bodyReceived < contentLength)
				return;  // Keep receiving body

			// Now full request is received
			HttpRequest httpRequest;
			httpRequest.parseRequest(_clients[fd].requestData);
			*/

			// Full request is received, prepare response

			HttpRequest httpRequest;
			httpRequest.parseRequest(_clients[fd].requestData);
			if(!httpRequest.getStatus()){
				HttpResponse errorResponse(httpRequest);
				errorResponse.generateResponse(400);

				_clients[fd].bytesSent = 0;
				_clients[fd].state = SENDING_RESPONSE;
				_pollFds[fd].events = POLLOUT;
				_clients[fd].shouldClose = true;
			}
			else{
				Methods method = stringToMethod(httpRequest.getMethod());
				switch (method) {
					case GET: handleGET(httpRequest, _clients[fd]); break;
					case POST: handlePOST(httpRequest, _clients[fd]); break;
					case DELETE: handleDELETE(httpRequest, _clients[fd]); break;
				}

				_clients[fd].bytesSent = 0;
				_clients[fd].state = SENDING_RESPONSE;
				_pollFds[fd].events = POLLOUT;
				std::cout << "Switched FD " << fd << " to POLLOUT mode (ready to send response)" << std::endl;
			}
		}
	}
	else if (revents & POLLOUT && _clients[fd].state == SENDING_RESPONSE){

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
			if (_clients[fd].bytesSent == _clients[fd].responseData.length()) {

				if(_clients[fd].shouldClose)
					std::cout << "Complete response sent to FD " << fd << ". Closing connection." << std::endl;
					disconectClient(fd);
					return;

				// Reset client state for next request
				_clients[fd].state = READING_REQUEST;
				_clients[fd].bytesSent = 0;
				_clients[fd].responseData.clear();

				// Switch back to reading mode
				for (size_t i = 0; i < _pollFds.size(); ++i) {
					if (_pollFds[i].fd == fd) {
						_pollFds[i].events = POLLIN;
						break;
					}
				}
			} else {
				std::cout << "Partial send: " << _clients[fd].bytesSent << "/" << _clients[fd].responseData.length() << " bytes sent" << std::endl;
			}

		} else {

			// Send failed, close connection
			std::cout << "Send failed for FD " << fd << ". Closing connection." << std::endl;
			disconectClient(fd);
		}
	}

	// Handle client disconnection
	if (revents & POLLHUP) {

		std::cout << "Client FD " << fd << " hung up" << std::endl;
		disconectClient(fd);
		std::cout << "Total FDs: " << _pollFds.size() << std::endl;
	}
}

void Server::disconectClient(short fd){

	close(fd);

	// Remove from arrays by shifting remaining elements
	_clients.erase(fd);
	for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
	{
		if(it->fd == fd){
			_pollFds.erase(it);
			break;
		}
	}

}

std::string Server::mapPath(const HttpRequest& request){

	std::string localPath = "/Users/antonsplavnik/Documents/Programming/42/Core/5/WebServ";
	std::string requestPath = request.getPath();
	return localPath + requestPath;
}


void Server::handleGET(const HttpRequest& request, ClientInfo& client){

	std::string mappedPath = mapPath(request);
	std::ifstream file(mappedPath.c_str());

	HttpResponse response(request, GET);
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

void Server::handlePOST(const HttpRequest& request, ClientInfo& client){

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

	return (path == "/Users/antonsplavnik/Documents/Programming/42/Core/5/WebServ/Downloads");
}
/**
 * This function only handles files, but not directories.
 */
void Server::handleDELETE(const HttpRequest& request, ClientInfo& client) {

	std::string mappedPath = mapPath(request);
	HttpResponse response(request, DELETE);
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
			std::cout << "Ok" << std::endl;
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

bool Server::isClientTimedOut(int fd){

	time_t now = time(NULL);
	if (now - _clients[fd].lastActivity > _clients[fd].keepAliveTimeout) true;
	return false;
}

void Server::updateClientActivity(int fd){

	_clients[fd].lastActivity = time(NULL);
}

void Server::checkClientTimeouts(){

	for(std::map<int, ClientInfo>::iterator it = _clients.begin(); it != _clients.end(); ++it){

		int currentFd = it->first;
		if(isClientTimedOut(currentFd)){
			std::cout << "Client: " << currentFd << " timed out." << std::endl;
			disconectClient(currentFd);
		}
	}
}
