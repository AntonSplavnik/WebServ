/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:30 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/05 17:49:12 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <map>
#include <cstring>
#include <cerrno>
#include <poll.h>
#include <ctime>
#include <fstream>
#include <arpa/inet.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>

#include "config.hpp"
#include "socket.hpp"
#include "client_info.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "post_handler.hpp"
#include "cgi.hpp"

class EventLoop;

#define BUFFER_SIZE_4 4096    // 4 KB
// #define BUFFER_SIZE_8 8192    // 8 KB
// #define BUFFER_SIZE_16 16384   // 16 KB
#define BUFFER_SIZE_32 32768   // 32 KB
// #define BUFFER_SIZE_65 65536   // 64 KB

class Server {

	public:
		Server(const ConfigData& config, EventLoop& controller);
		~Server();

		void handleEvent(int fd, short revents);
		void disconnectClient(short fd);
		void handleCGItimeout(Cgi* cgi);


		const std::vector<Socket>& getListeningSockets() const;
		std::map<int, ClientInfo>& getClients();
		std::map<int, Cgi*>& getCGI();

	private:
		void initListeningSockets();

		void handleListenEvent(int fd);

		void handleCGIerror(int fd);
		void handleCGIread(int fd);
		void handleCGIwrite(int fd);

		void handleClientRead(int indexOfLinstenSocket);
		void handleClientWrite(int fd);

		void handleCGI();

		void handleGET(const HttpRequest& request, ClientInfo& client, std::string mappedPath);
		void handlePOST(const HttpRequest& request, ClientInfo& client, std::string mappedPath);
		void handleDELETE(const HttpRequest& request, ClientInfo& client, std::string mappedPath);

		bool validateMethod(const HttpRequest& request, const LocationConfig*& location);
		std::string mapPath(const HttpRequest& request, const LocationConfig*& matchedLocation);
		bool isPathSafe(const std::string& mappedPath, const std::string& allowedRoot);

		void shutdown();
		void terminateCGI(Cgi* cgi);
		int isListeningSocket(int fd) const;
		void updateClientActivity(int fd);

		// Utility
		// void logConnection(const Client& client);
		// void logDisconnection(int client_fd);

		std::vector<Socket>			_listeningSockets;
		std::map<int, ClientInfo>	_clients;
		std::map<int, Cgi*>			_cgi;
		const ConfigData&			_configData;
		EventLoop&					_controller;
};

#endif
