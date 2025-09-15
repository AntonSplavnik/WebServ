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
	_maxClients(1000),
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
			std::cout << "New connection accepted! Client FD: " << client_fd << std::endl;

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
			close(client_fd);  // Just close, nothing to remove
		}
	}
}

void Server::handleClientSocket(short fd, short revents){

	if(revents & POLLIN && _clients[fd].state == READING_REQUEST){

		char buffer[BUFFER_SIZE];
		std::memset(buffer, 0, BUFFER_SIZE);
		int bytes = recv(fd, buffer, BUFFER_SIZE - 1, 0);

		std::cout << "recv() returned " << bytes << " bytes from FD " << fd << std::endl;

		if (bytes == 0 || (bytes < 0 && !revents & POLLIN && !_clients[fd].state == READING_REQUEST)) {

			// Client disconnected or error
			clientDisconetion(fd);
			if(bytes = 0 )
				std::cout << "Client FD " << fd << " disconnected" << std::endl;
			else
				std::cout << "Client FD " << fd << " disconnected" << std::endl;
		}
		else {

		}
	}
}

void Server::clientDisconetion(short fd){

	close(fd);
		// Remove from arrays by shifting remaining elements
		_clients.erase(fd);
		for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
		{
			if(it->fd == fd)
			it->fd = it->fd + 1;
		}

}
