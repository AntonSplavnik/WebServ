/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server_controller.hpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/06 13:18:43 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/11 16:21:58 by antonsplavn      ###   ########.fr       */
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

		void addServers();
		void run();

	private:

		void stop();
		Server* findServerForFd(int fd);
		void initListeningSockets();
		void rebuildPollFds();
		void checkClientTimeouts(Server& server);
		bool isClientTimedOut(std::map<int, ClientInfo>& clients, int fd);

		std::vector<Server*> _servers;
		std::vector<struct pollfd> _pollFds;
		std::vector<ConfigData> _configs;

		size_t _listeningSocketCount;
		bool _running;
};

#endif
