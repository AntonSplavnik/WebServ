#ifndef LIESTENING_SOCKET_MANAGER
#define LIESTENING_SOCKET_MANAGER

#include "socket.hpp"
#include "config.hpp"
#include "connection.hpp"
#include "connection_pool_manager.hpp"

#include <arpa/inet.h>
#include <poll.h>


#define MAX_CLIENTS 256 //  ulimit -n

class ListeningSocketManager {

	public:
		ListeningSocketManager(): _fd() {}
		~ListeningSocketManager() {
			for (size_t i = 0; i < _listeningSockets.size(); i++) {
				int fd = _listeningSockets[i].getFd();
				if (fd >= 0) {
					close(fd);
				}
			}
		}

		void initListeningSockets(std::vector<ConfigData>& configs);
		void handleListenEvent(int fd, short revents, ConnectionPoolManager& connectionPoolManager);
		bool isListening(int fd);

		std::vector<int>& getListeningSockets() { return _fd;}

	private:
		std::vector<Socket>		_listeningSockets;
		std::vector<int>		_fd;
};

#endif
