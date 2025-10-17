/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:30 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/14 20:42:13 by antonsplavn      ###   ########.fr       */
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
<<<<<<< HEAD
#include "../socket/socket.hpp"
#include "../config/config.hpp"
#include "../http_request/http_request.hpp"
#include "../http_response/http_response.hpp"
#include "../cgi/cgi.hpp"
=======
#include "socket.hpp"
#include "client_info.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "post_handler.hpp"
#include "config.hpp"
>>>>>>> 79fcc5d960ccba1cbf6e0d85b402c4962da74f69

class Server {

	public:
		Server(const ConfigData& config);
		~Server();

		void handleEvent(int fd, short revents);
		void disconectClient(short fd);
		void shutdown();

		const std::vector<Socket>& getListeningSockets() const;
		std::map<int, ClientInfo>& getClients();
        std::map<int, Cgi*>& getCgiMap();

	private:

		void initializeListeningSockets();

		int isListeningSocket(int fd) const;
		void handlePOLLERR(int fd);
		void handlePOLLHUP(int fd);
		void handleListenEvent(int fd);
		void handleClientRead(int indexOfLinstenSocket);
		void handleClientWrite(int fd);

		void handleGET(const HttpRequest& request, ClientInfo& client);
		void handlePOST(const HttpRequest& request, ClientInfo& client);
		void handleDELETE(const HttpRequest& request, ClientInfo& client);

		bool validatePath(std::string path);
		std::string mapPath(const HttpRequest& request);
		void updateClientActivity(int fd);	// Reset timer on activity

                // CGI handling
		void handleCgiCompletion(int fd);

		// Utility
		// void logConnection(const Client& client);
		// void logDisconnection(int client_fd);

		// Config _config;
		std::vector<Socket>			 _listeningSockets;
		std::map<int, ClientInfo>	_clients;
	    std::map<int, Cgi*>			_cgiMap;
		const ConfigData			_configData;
};

#endif

		// Server lifecycle

		// Connection management
		// void acceptNewClient();
		// void disconnectClient(int client_fd);
		// void addToPoll(int fd, short events);
		// void removeFromPoll(int fd);

		// Client handling
		// void handleClientData(int client_fd);
		// void handleClientWrite(int client_fd);
		// void processClientRequest(int client_fd);

		// Methods handeling

		// Configuration
		// void setPort(int port);
		// void setHost(const std::string& host);
		// void setMaxClients(int max_clients);

		// Getters
		// int getPort() const;
		// std::string getHost() const;
		// bool isRunning() const;
		// size_t getClientCount() const;

		// Utility
		// void cleanup();
		// void logConnection(const Client& client);
		// void logDisconnection(int client_fd);
