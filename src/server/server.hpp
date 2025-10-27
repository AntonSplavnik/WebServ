/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:30 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/27 23:12:20 by antonsplavn      ###   ########.fr       */
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

// #define BUFFER_SIZE 4096    // 4 KB
// #define BUFFER_SIZE 8192    // 8 KB
// #define BUFFER_SIZE 16384   // 16 KB
#define BUFFER_SIZE 32768   // 32 KB
// #define BUFFER_SIZE 65536   // 64 KB

class Server {

	public:
		Server(const ConfigData& config);
		~Server();

		void handleEvent(int fd, short revents);
		void disconnectClient(short fd);
		void terminateCGI(int fd);
		void shutdown();

		const std::vector<Socket>& getListeningSockets() const;
		std::map<int, ClientInfo>& getClients();
		std::map<int, Cgi*>& getCGI();

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
};

#endif
