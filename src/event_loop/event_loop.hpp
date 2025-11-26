/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   event_loop.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/06 13:18:43 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/26 02:07:18 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_MANAGER
#define SERVER_MANAGER

#include <poll.h>

#include "cgi_executor.hpp"
#include "listening_socket_manager.hpp"
#include "connection_pool_manager.hpp"
#include "connection.hpp"
#include "config.hpp"
#include "cgi.hpp"
#include "server.hpp"

class EventLoop {

	public:
		EventLoop::EventLoop(Config& config)
			:_configs(config.getServers()),
			 _connectionPoolManager(_configs),
			 _cgiExecutor(*this),
			 _listenManager(),
			 _listeningSocketCount(),
			 _running(true) {}
		EventLoop::~EventLoop() { stop(); }

		void run();
		void addKilledPid(pid_t pid);


	private:
		void stop();
		void initListeningSockets();
		void rebuildPollFds();

		bool isConnectionTimedOut(std::map<int, Connection> &connections, int fd);
		void checkConnectionsTimeouts();

		bool isCgiTimedOut(std::map<int, Cgi>& cgiMap, int fd);
		void checkCgiTimeouts();

		void EventLoop::processDiskWrites();

		void reapZombieProcesses();

		std::vector<ConfigData>		_configs;
		ConnectionPoolManager		_connectionPoolManager;
		ListeningSocketManager		_listenManager;
		CgiExecutor					_cgiExecutor;

		std::vector<struct pollfd>	_pollFds;
		std::vector<int>			_killedPids;

		size_t						_listeningSocketCount;
		bool						_running;
};

#endif
