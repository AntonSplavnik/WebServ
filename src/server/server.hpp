/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:30 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/09/25 18:31:53 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "socket.hpp"
#include <vector>
#include <map>
#include <poll.h>
#include "http_request.hpp"
#include "http_response.hpp"

enum Methods {
	GET,
	POST,
	DELETE
};

// Client connection states
enum ClientState {
	READING_REQUEST,   // Waiting to read HTTP request
	SENDING_RESPONSE   // Ready to send HTTP response
};

// Structure to track client connection info
struct ClientInfo {
	ClientInfo() : socket(), state(READING_REQUEST), bytesSent(0) {}
	ClientInfo(int fd) : socket(fd), state(READING_REQUEST), bytesSent(0) {}
	Socket socket;
	ClientState state;
	size_t bytesSent;
	std::string responseData;
	std::string requstData;
	//timeout data
	time_t lastActivity;        // Last time client sent data
	int keepAliveTimeout;       // Timeout in seconds (default 15)
	int maxRequests;           // Max requests per connection
	int requestCount;          // Current request count

};


class Server {

	public:
		Server();
		// Server(Config config);
		~Server();

		// // Server lifecycle
		void initialize();
		void start();
		void stop();
		void run();

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
		Methods stringToMethod(const std::string& method);
		void handleGET(const HttpRequest& request, ClientInfo& client);
		void handlePOST(const HttpRequest& request, ClientInfo& client);
		void handleDELETE(const HttpRequest& request, ClientInfo& client);

		// Event loop
		void handlePollEvents();
		void handleServerSocket(size_t index);
		void handleClientSocket(short fd, short revents);

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
		bool validatePath(std::string path);
		void clientDisconetion(short fd);
		std::string Server::mapPath(const HttpRequest& request);
		bool isClientTimedOut(int fd);  // Check specific client
		void updateClientActivity(int fd);	// Reset timer onactivity
		void checkClientTimeouts();	// Check all clients fortimeout
		// void cleanup();
		// void logConnection(const Client& client);
		// void logDisconnection(int client_fd);

	private:
		// Config _config;
		Socket _serverSocket;
		std::vector<struct pollfd> _pollFds;
		std::map<int, ClientInfo> _clients;
		std::string _host;
		int _port;
		bool _running;
		size_t _maxClients;
};

#endif
