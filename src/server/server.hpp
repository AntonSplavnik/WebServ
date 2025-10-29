/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:30 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/29 16:18:27 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <map>
#include <cstring>
#include <cerrno>
#include <poll.h>
#include "socket.hpp"
#include "client_info.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "post_handler.hpp"
#include "config.hpp"

class Server {

	public:
		Server(const ConfigData& config);
		~Server();

		void handleEvent(int fd, short revents);
		void disconectClient(short fd);
		void shutdown();

		const std::vector<Socket>& getListeningSockets() const;
		std::map<int, ClientInfo>& getClients();

	private:



		void handleListenEvent(int fd);
		void handleClientRead(int indexOfLinstenSocket);
		void handleClientWrite(int fd);

		void handleGET(const HttpRequest& request, ClientInfo& client, std::string mappedPath);
		void handlePOST(const HttpRequest& request, ClientInfo& client, std::string mappedPath);
		void handleDELETE(const HttpRequest& request, ClientInfo& client, std::string mappedPath);

		bool validateMethod(const HttpRequest& request, const LocationConfig*& location);
		std::string mapPath(const HttpRequest& request, const LocationConfig*& matchedLocation);
		bool isPathSafe(const std::string& mappedPath, const std::string& allowedRoot);

		void initializeListeningSockets();
		int isListeningSocket(int fd) const;
		void updateClientActivity(int fd);
		// Utility
		// void logConnection(const Client& client);
		// void logDisconnection(int client_fd);

		std::vector<Socket>			_listeningSockets;
		std::map<int, ClientInfo>	_clients;
		const ConfigData			_configData;
};

#endif
