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

class EventLoop; // Forward declaration
class CgiExecutor; // Forward declaration
class SessionManager; // Forward declaration

class ConnectionPoolManager {

	public:
		ConnectionPoolManager(std::vector<ConfigData>& configs)
			: _configs(configs) {}
		~ConnectionPoolManager(){}

		void handleConnectionEvent(int fd, short revents, CgiExecutor& cgiExecutor, SessionManager& sessionManager);
		Connection* getConnection(int fd);
		Connection& getConnectionRef(int fd);
		void addConnection(Connection& incomingConnection);
		void disconnectConnection(short fd);
		bool isConnection(int fd);

		std::map<int, Connection>& getConnectionPool() {return _connectionPool;}

	private:
		std::map<int, Connection>	_connectionPool;
		std::vector<ConfigData>&	_configs;
};

#endif
