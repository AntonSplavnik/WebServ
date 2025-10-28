/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server_controller.hpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/06 13:18:43 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/29 00:01:44 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_MANAGER
#define SERVER_MANAGER

#include "../server/server.hpp"
#include "../config/config.hpp"
#include "../cgi/cgi.hpp"

class ServerController{

	public:
		ServerController(Config& config);
		~ServerController();

		void run();
		void addKilledPid(pid_t pid);

		private:

		void stop();
		void addServers();
		void initListeningSockets();
		void rebuildPollFds();
		Server* findServerForFd(int fd);
		bool isClientTimedOut(std::map<int, ClientInfo>& clients, int fd);
		void checkClientTimeouts(Server& server);
		bool isCgiTimedOut(std::map<int, Cgi*>& cgiMap, int fd);
		void checkCgiTimeouts(Server& server);
		void reapZombieProcesses();

		std::vector<Server*> 		_servers;
		std::vector<struct pollfd>	_pollFds;
		std::vector<ConfigData>		_configs;
		std::vector<int>			_killedPids;

		size_t						_listeningSocketCount;
		bool						_running;
};

#endif
