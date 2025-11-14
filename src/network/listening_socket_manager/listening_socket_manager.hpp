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
		ListeningSocketManager(ConnectionPoolManager& connectionPoolManager);
		~ListeningSocketManager();

		void initListeningSockets(std::vector<ConfigData>& configs);
		void handleListenEvent(int fd, short revents);

		bool isListening(int fd);

		std::vector<int>& getListeningSockets() { return _fd;}

	private:
		ConnectionPoolManager&	_conPoolManager;
		std::vector<Socket>		_listeningSockets;  // probably remove, not needed
		std::vector<int>		_fd;

};

#endif
