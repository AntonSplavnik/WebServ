#ifndef CONNECTION_POOL_MANAGER
#define CONNECTION_POOL_MANAGER

#include <poll.h>

#include "connection.hpp"

class ConnectionPoolManager {

	public:
		ConnectionPoolManager();
		~ConnectionPoolManager();

		// comment from Maksim: maybe move to private?
		void handleConnectionEvent(int fd, short revents); 
		void addConnection(Connection& incomingConnection);
		void disconnectConnection(short fd);
		bool isConnection(int fd);

		std::map<int, Connection>& getConnectionPool() {return _connectionPool;}

	private:

		std::map<int, Connection>	_connectionPool;

};

#endif
