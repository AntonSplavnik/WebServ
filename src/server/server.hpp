/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:30 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/01 19:57:30 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <map>
#include <cstring>
#include <cerrno>
#include <poll.h>
#include "../socket/socket.hpp"
#include "../config/config.hpp"
#include "../http_response/http_response.hpp"
#include "../cgi/cgi.hpp"

class ServerController;

#define BUFFER_SIZE_4 4096    // 4 KB
// #define BUFFER_SIZE_8 8192    // 8 KB
// #define BUFFER_SIZE_16 16384   // 16 KB
#define BUFFER_SIZE_32 32768   // 32 KB
// #define BUFFER_SIZE_65 65536   // 64 KB

class Server {

	public:
		Server(const ConfigData& config, ServerController& controller);
		~Server();

		void handleEvent(int fd, short revents);
		void disconnectClient(short fd);
		void handleCGItimeout(Cgi* cgi);

		const std::vector<Socket>& getListeningSockets() const;
		std::map<int, ClientInfo>& getClients();
		std::map<int, Cgi*>& getCGI();
		void testFunction();
	private:
		void initListeningSockets();

		void handleListenEvent(int fd);

		void handleCGIerror(int fd);
		void handleCGIread(int fd);
		void handleCGIwrite(int fd);

		void handleClientRead(int fd);
		void handleClientWrite(int fd);

		void handleGET(const HttpRequest& request, ClientInfo& client, const LocationConfig* matchedLoc);
		void handlePOST(const HttpRequest& request, ClientInfo& client, const LocationConfig* matchedLoc);
		void handleDELETE(const HttpRequest& request, ClientInfo& client, const LocationConfig* matchedLoc);

		void shutdown();
		void terminateCGI(Cgi* cgi);
		int isListeningSocket(int fd) const;
		bool validatePath(const std::string& path);
		void updateClientActivity(int fd);

		// Utility
		// void logConnection(const Client& client);
		// void logDisconnection(int client_fd);

		std::vector<Socket>			_listeningSockets;
		std::map<int, ClientInfo>	_clients;
		std::map<int, Cgi*>			_cgi;
		const ConfigData			_configData;
		ServerController&			_controller;
};

#endif
