#include "server.hpp"
#include "config.hpp"
#include "socket.hpp"


/* - initialize() - Create and configure the listening socket using your
   Socket class
  - start() - Set up poll array, add listening socket to poll
  - run() - Main event loop with poll()
  - stop() - Cleanup */

Server::Server()
	: _running(false),
	_maxClients(MAX_CON_BACKLOG),
	_port(8080),
	_host("0.0.0.0"){

	_pollFds.clear();
	_clients.clear();
	// _pollFds.reserve(_maxClients + 1); (optimisation)
}

Server::~Server(){}

void Server::initialize(){

	_serverSocket.createDefault();
	_serverSocket.setReuseAddr(true);
	_serverSocket.binding(_config.GetConfigData().port);
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
	}
}


void Server::handlePollEvents(){

/*
Then implement:
- handlePollEvents() - Check each fd in _pollFds
- handleServerSocket() - Accept new connections
- handleClientSocket() - Process client data
*/


if (_pollFds.data()->revents & POLLIN) {
		std::cout << "Event detected on listening socket FD " << _pollFds.data()->fd << std::endl;

		// Accept the new connection
		ClientInfo client;
		int client_fd = _serverSocket.accepting();

		if (client.fd >= 0 && MAX_CON_BACKLOG < 10) {
			std::cout << "New connection accepted! Client FD: " << client.fd << std::endl;

			// Make client socket non-blocking
			make_socket_non_blocking(client.fd);

			// Add client socket to poll array
			fds[nfds].fd = client_fd;
			fds[nfds].events = POLLIN;  // Start by reading the request

			// Initialize client info
			clients[nfds].fd = client_fd;
			clients[nfds].state = READING_REQUEST;

			std::cout << "Added client socket FD " << client_fd << " to poll array at index " << nfds << std::endl;
			nfds++;
			std::cout << "New client connected. Total FDs: " << nfds << std::endl;
		}
	}

}
