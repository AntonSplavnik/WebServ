#ifndef CONNECTION_POOL_MANAGER
#define CONNECTION_POOL_MANAGER

#include <poll.h>

#include "connection.hpp"

class EventLoop; // Forward declaration

class ConnectionPoolManager {

	public:
		ConnectionPoolManager(std::vector<ConfigData>& configs)
			: _configs(configs){}
		~ConnectionPoolManager(){}

		void handleConnectionEvent(int fd, short revents, CgiExecutor& cgiExecutor);
		Connection* getConnection(int fd);
		void addConnection(Connection& incomingConnection);
		void disconnectConnection(short fd);
		bool isConnection(int fd);

		std::map<int, Connection>& getConnectionPool() {return _connectionPool;}

	private:
		std::map<int, Connection>	_connectionPool;
		std::vector<ConfigData>&	_configs;
};

#endif
