/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   connection_pool_manager.hpp                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/26 11:18:50 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/27 13:56:46 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONNECTION_POOL_MANAGER_HPP
#define CONNECTION_POOL_MANAGER_HPP

#include <poll.h>

#include "connection.hpp"
#include "logger.hpp"

class EventLoop; // Forward declaration
class CgiExecutor; // Forward declaration

class ConnectionPoolManager {

	public:
		ConnectionPoolManager(std::vector<ConfigData>& configs);
		~ConnectionPoolManager();

		void handleConnectionEvent(int fd, short revents, CgiExecutor& cgiExecutor);
		Connection* getConnection(int fd);
		Connection& getConnectionRef(int fd);
		void addConnection(Connection& incomingConnection);
		void disconnectConnection(short fd);
		bool isConnection(int fd);

		std::map<int, Connection>& getConnectionPool() {return _connectionPool;}
		Logger* getLoggerForConfig(ConfigData* config);

	private:
		std::map<int, Connection>	_connectionPool;
		std::vector<ConfigData>&	_configs;
		std::map<ConfigData*, Logger*>	_loggers; // One logger per server config
};

#endif
