#include "server.hpp"
#include "../config/config.hpp"
#include "../socket/socket.hpp"
#include <cstring>
#include <cerrno>


/* - initialize() - Create and configure the listening socket using your
   Socket class
  - start() - Set up poll array, add listening socket to poll
  - run() - Main event loop with poll()
  - stop() - Cleanup */

Server::Server()
	: _host("0.0.0.0"),
	_port(8080),
	_running(false),
	_maxClients(MAX_CON_BACKLOG){

	_pollFds.clear();
	_clients.clear();
	// _pollFds.reserve(_maxClients + 1); (optimisation)
}

Server::Server(Config config)
	: _config(config),
	_host("0.0.0.0"),
	_port(config.GetConfigData().port),
	_running(false),
	_maxClients(MAX_CON_BACKLOG){

	_pollFds.clear();
	_clients.clear();
}

Server::~Server(){}

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

	std::cout << "Entering main event loop..." << std::endl;
	while(_running){

		std::cout << "Calling poll() with " << _pollFds.size() << " file descriptors..." << std::endl;
		int ret = poll(_pollFds.data(), _pollFds.size(), -1);
		
		if (ret < 0) {
			std::cerr << "Poll failed: " << strerror(errno) << std::endl;
				break;
		}
		
		if (ret == 0) {
			std::cout << "Poll timeout (this shouldn't happen with -1)" << std::endl;
			continue;
		}

		std::cout << "poll() returned " << ret << " (number of FDs with events)" << std::endl;

		handlePollEvents();
	}
}


void Server::handlePollEvents(){

/*
Then implement:
- handlePollEvents() - Check each fd in _pollFds
- handleServerSocket() - Accept new connections
- handleClientSocket() - Process client data
*/

	std::cout << "Checking " << _pollFds.size() << " file descriptors for events..." << std::endl;

	for (size_t i = 0; i < _pollFds.size(); i++){

		std::cout << "Checking FD " << _pollFds[i].fd << " at index " << i << " - events: " << _pollFds[i].events << " revents: " << _pollFds[i].revents << std::endl;

		if (_pollFds[i].revents == 0) {
			std::cout << "No events on FD " << _pollFds[i].fd << ", skipping..." << std::endl;
			continue;
		}

		if(i == 0) {
			std::cout << "Processing server socket (index 0)" << std::endl;
			handleServerSocket(i);
		} else {
			std::cout << "Processing client socket FD " << _pollFds[i].fd << std::endl;
			handleClientSocket(_pollFds[i].fd, _pollFds[i].revents);
		}
	}

}

void Server::handleServerSocket(size_t index){

	std::cout << "handleServerSocket called for index " << index << " FD " << _pollFds[index].fd << std::endl;
	std::cout << "revents = " << _pollFds[index].revents << " (POLLIN=" << POLLIN << ")" << std::endl;

	if (_pollFds[index].revents & POLLIN){
		std::cout << "POLLIN event detected on listening socket FD " << _pollFds[index].fd << std::endl;

		//Accept the new connection
		std::cout << "Calling accept()..." << std::endl;
		short client_fd = _serverSocket.accepting();
		
		if (client_fd >= 0) {
			std::cout << "New connection accepted! Client FD: " << client_fd << std::endl;
			
			// Create client info and add to map
			_clients[client_fd] = ClientInfo(client_fd);

			// Make client socket non-blocking
			_clients[client_fd].socket.setNonBlocking();
			std::cout << "Making socket FD " << client_fd << " non-blocking" << std::endl;

			// Add client socket to poll array
			struct pollfd clientPollFd;
			clientPollFd.fd = client_fd;
			clientPollFd.events = POLLIN;
			clientPollFd.revents = 0;
			_pollFds.push_back(clientPollFd);

			std::cout << "Added client socket FD " << client_fd << " to poll vector at index " << _pollFds.size()-1 << std::endl;
			std::cout << "New client connected. Total FDs: " << _pollFds.size() << std::endl;
		} else {
			std::cout << "Failed to accept new connection" << std::endl;
		}
	} else {
		std::cout << "No POLLIN event on server socket (revents=" << _pollFds[index].revents << ")" << std::endl;
	}
}

void Server::handleClientSocket(short fd, short revents){

	if(revents & POLLIN && _clients[fd].state == READING_REQUEST){

		char buffer[BUFFER_SIZE];
		std::memset(buffer, 0, BUFFER_SIZE);
		int bytes = recv(fd, buffer, BUFFER_SIZE - 1, 0);

		std::cout << "recv() returned " << bytes << " bytes from FD " << fd << std::endl;

		if (bytes == 0) {
			// Client disconnected gracefully
			std::cout << "Client FD " << fd << " disconnected gracefully" << std::endl;
			disconnectClient(fd);
		}
		else if (bytes < 0) {
			// Error occurred
			std::cout << "Error reading from client FD " << fd << std::endl;
			disconnectClient(fd);
		}
		else {
			// Successfully received data
			buffer[bytes] = '\0';
			std::cout << "Received " << bytes << " bytes from client FD " << fd << std::endl;
			std::cout << "Data: " << buffer << std::endl;
			
			// TODO: Parse HTTP request and prepare response
			// For now, just echo back a simple HTTP response
			std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello World!\n";
			_clients[fd].responseData = response;
			_clients[fd].state = SENDING_RESPONSE;
			
			// Modify poll events to watch for POLLOUT
			for (size_t i = 0; i < _pollFds.size(); i++) {
				if (_pollFds[i].fd == fd) {
					_pollFds[i].events = POLLOUT;
					break;
				}
			}
		}
	}
	else if (revents & POLLOUT && _clients[fd].state == SENDING_RESPONSE) {
		// Handle sending response
		handleClientWrite(fd);
	}
}

// Disconnect a client and clean up resources
void Server::disconnectClient(int client_fd) {
	std::cout << "Disconnecting client FD " << client_fd << std::endl;
	
	// Close the socket
	close(client_fd);
	
	// Remove from clients map
	_clients.erase(client_fd);
	
	// Remove from poll array
	removeFromPoll(client_fd);
}

// Remove a file descriptor from the poll array
void Server::removeFromPoll(int fd) {
	for (size_t i = 0; i < _pollFds.size(); i++) {
		if (_pollFds[i].fd == fd) {
			std::cout << "Removing FD " << fd << " from poll vector at index " << i << std::endl;
			_pollFds.erase(_pollFds.begin() + i);
			break;
		}
	}
}

// Handle writing response to client
void Server::handleClientWrite(int client_fd) {
	ClientInfo& client = _clients[client_fd];
	
	const char* data = client.responseData.c_str() + client.bytesSent;
	size_t remaining = client.responseData.length() - client.bytesSent;
	
	ssize_t bytes_sent = send(client_fd, data, remaining, 0);
	
	if (bytes_sent > 0) {
		client.bytesSent += bytes_sent;
		std::cout << "Sent " << bytes_sent << " bytes to client FD " << client_fd 
				  << " (" << client.bytesSent << "/" << client.responseData.length() << ")" << std::endl;
		
		// Check if we've sent the entire response
		if (client.bytesSent >= client.responseData.length()) {
			std::cout << "Response fully sent to client FD " << client_fd << std::endl;
			disconnectClient(client_fd);
		}
	} else if (bytes_sent == 0) {
		std::cout << "Client FD " << client_fd << " closed connection" << std::endl;
		disconnectClient(client_fd);
	} else {
		std::cout << "Error sending to client FD " << client_fd << ": " << strerror(errno) << std::endl;
		disconnectClient(client_fd);
	}
}

// Setter methods
void Server::setPort(int port) {
	_port = port;
}

void Server::setHost(const std::string& host) {
	_host = host;
}

void Server::setMaxClients(int max_clients) {
	_maxClients = max_clients;
}

// Getter methods
int Server::getPort() const {
	return _port;
}

std::string Server::getHost() const {
	return _host;
}

bool Server::isRunning() const {
	return _running;
}

size_t Server::getClientCount() const {
	return _clients.size();
}

// Stop the server
void Server::stop() {
	std::cout << "Stopping server..." << std::endl;
	_running = false;
	
	// Close all client connections
	for (std::map<int, ClientInfo>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		close(it->first);
	}
	_clients.clear();
	
	// Close server socket
	if (_serverSocket.getFd() >= 0) {
		close(_serverSocket.getFd());
	}
	
	_pollFds.clear();
}

// Utility methods
void Server::cleanup() {
	stop();
}

void Server::logConnection(int client_fd) {
	std::cout << "New client connected: FD " << client_fd << std::endl;
}

void Server::logDisconnection(int client_fd) {
	std::cout << "Client disconnected: FD " << client_fd << std::endl;
}
