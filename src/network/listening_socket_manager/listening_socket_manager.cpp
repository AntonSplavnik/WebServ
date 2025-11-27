#include "listening_socket_manager.hpp"

void ListeningSocketManager::initListeningSockets(std::vector<ConfigData>& configs) {

	for (size_t i = 0; i < configs.size(); i++)
	{
		//add logic for incoming listening sockets from the config file.
		for(size_t j = 0; j < configs[i].listeners.size(); j++) {

			Socket listeningSocket;

			listeningSocket.createDefault();

			int fd = listeningSocket.getFd();

			if (fd < 0) {
				throw std::runtime_error("[DEBUG] Failed to create socket");
			}

			_fd.push_back(fd);

			listeningSocket.setReuseAddr(true);

			std::string address = configs[i].listeners[j].first;
			unsigned short port = configs[i].listeners[j].second;
			listeningSocket.binding(address, port);
			if (fd < 0) {
				throw std::runtime_error("[DEBUG] Failed to bind socket (port may be in use)");
			}

			listeningSocket.listening(configs[i].backlog);
			if (fd < 0) {
				throw std::runtime_error("[DEBUG] Failed to listen on socket");
			}

			Socket::setNonBlocking(fd);

			_listeningSockets.push_back(listeningSocket);

			std::cout << "[DEBUG] Susesfully added listening socket fd: "
					  << fd << " at vector position: "
					  << i << std::endl;
		}
	}
}
void ListeningSocketManager::handleListenEvent(int fd, short revents, ConnectionPoolManager& connectionPoolManager) {

	if (revents & POLLIN) {
		std::cout << "[DEBUG] Event detected on listening socket FD " << fd << std::endl;

		Socket* listeningSocket = NULL;
		for (size_t i = 0; i < _listeningSockets.size(); i++)
		{
			if (_listeningSockets[i].getFd() == fd){
				listeningSocket = &_listeningSockets[i];
				break;
			}
		}
		if (!listeningSocket){
			std::cerr << "[ERROR] Unknown listening socket FD: " << fd << std::endl;
			return;
		}

		if (connectionPoolManager.getConnectionPool().size() >= static_cast<size_t>(MAX_CLIENTS)){
			std::cout << "[DEBUG] Max clients reached, rejecting connection" << std::endl;
			return;
		}

		sockaddr_in clientAddr;
		int clientFd = listeningSocket->accepting(clientAddr);

		if (clientFd < 0){
			std::cout << "[DEBUG] Error on FD accept" << std::endl;
			return;

		Socket::setNonBlocking(clientFd);

		Connection incomingConnection(
			clientFd,
			inet_ntoa(clientAddr.sin_addr),
			ntohs(clientAddr.sin_port),
			listeningSocket->getPort()
		);
		connectionPoolManager.addConnection(incomingConnection);

		std::cout << "[DEBUG] New connection accepted! Client FD: " << clientFd
					<< "Timeout: 15" << "Max Max Requests: 100" << std::endl;
		}
	}
}

bool ListeningSocketManager::isListening(int fd) {
	for (size_t i = 0; i < _fd.size(); i++) {
		if (_fd[i] == fd)
			return true;
	}
	return false;
}
