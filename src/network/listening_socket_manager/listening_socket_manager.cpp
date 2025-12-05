#include "listening_socket_manager.hpp"
#include "logger.hpp"
#include <algorithm>
#include <set>

void ListeningSocketManager::initListeningSockets(std::vector<ConfigData>& configs) {

	std::set<unsigned short> ports;

	for (size_t i = 0; i < configs.size(); i++)
	{

		//add logic for incoming listening sockets from the config file.
		for(size_t j = 0; j < configs[i].listeners.size(); j++) {

			unsigned short port = configs[i].listeners[j].second;
			if (!ports.insert(port).second){
				continue;
			}
			ports.insert(port);


			Socket listeningSocket;

			listeningSocket.createDefault();

			int fd = listeningSocket.getFd();

			if (fd < 0) {
				throw std::runtime_error("[DEBUG] Failed to create socket");
			}

			_fd.push_back(fd);
			logDebug("Successfully added fd: " + toString(fd) + " to vector<int> at vector position: " + toString(_fd.size()));

			listeningSocket.setReuseAddr(true);

			std::string address = configs[i].listeners[j].first;
			// unsigned short port = configs[i].listeners[j].second;
			listeningSocket.binding(address, port);
			if (listeningSocket.getFd() < 0) {
				throw std::runtime_error("[DEBUG] Failed to bind socket (port may be in use)");
			}

			listeningSocket.listening(configs[i].backlog);
			if (fd < 0) {
				throw std::runtime_error("[DEBUG] Failed to listen on socket");
			}

			Socket::setNonBlocking(fd);

			_listeningSockets.push_back(listeningSocket);

			logDebug("Successfully added fd: " + toString(fd) + " to vector<Socket> at vector position: " + toString(_listeningSockets.size()));
		}
	}
}
void ListeningSocketManager::handleListenEvent(int fd, short revents, ConnectionPoolManager& connectionPoolManager) {

	(void)revents;
	logDebug("Event detected on listening socket FD " + toString(fd));

	Socket* listeningSocket = NULL;
	for (size_t i = 0; i < _listeningSockets.size(); i++)
	{
		if (_listeningSockets[i].getFd() == fd){
			listeningSocket = &_listeningSockets[i];
			break;
		}
	}
	if (!listeningSocket){
		logError("Unknown listening socket FD: " + toString(fd));
		return;
	}

	if (connectionPoolManager.getConnectionPool().size() >= static_cast<size_t>(MAX_CLIENTS)){
		logWarning("Max clients reached, rejecting connection");
		return;
	}

	sockaddr_in clientAddr;
	int clientFd = listeningSocket->accepting(clientAddr);

	if (clientFd < 0){
		logDebug("Error on FD accept");
		return;
	}

	Socket::setNonBlocking(clientFd);

	Connection incomingConnection(
		clientFd,
		inet_ntoa(clientAddr.sin_addr),
		ntohs(clientAddr.sin_port),
		listeningSocket->getPort()
	);
	connectionPoolManager.addConnection(incomingConnection);

	logInfo("Client connected(fd = " + toString(clientFd) + ", ip = " + incomingConnection.getIp() + ")");

}

bool ListeningSocketManager::isListening(int fd) {
	for (size_t i = 0; i < _fd.size(); i++) {
		if (_fd[i] == fd)
			return true;
	}
	return false;
}
