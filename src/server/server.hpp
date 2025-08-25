#ifndef SERVER_HPP
#define SERVER_HPP

#include "socket.hpp"
#include <vector>
#include <map>
#include <poll.h>
#include "../config/config.hpp"


// Client connection states
enum ClientState {
	READING_REQUEST,   // Waiting to read HTTP request
	SENDING_RESPONSE   // Ready to send HTTP response
};

// Structure to track client connection info
struct ClientInfo {
	ClientInfo(int fd) : socket(fd), state(READING_REQUEST), bytesSent(0) {}
	Socket socket;
	ClientState state;
	std::string responseData;
	size_t bytesSent;
};


class Server {

	public:
		Server();
		Server(Config config);
		~Server();

		// // Server lifecycle
		void initialize();
		void start();
		void stop();
		void run();

		// Connection management
		void acceptNewClient();
		void disconnectClient(int client_fd);
		void addToPoll(int fd, short events);
		void removeFromPoll(int fd);

		// Client handling
		void handleClientData(int client_fd);
		void handleClientWrite(int client_fd);
		void processClientRequest(int client_fd);

		// Event loop
		void handlePollEvents();
		void handleServerSocket(size_t index);
		void handleClientSocket(short fd, short revents);

		// Configuration
		void setPort(int port);
		void setHost(const std::string& host);
		void setMaxClients(int max_clients);

		// Getters
		int getPort() const;
		std::string getHost() const;
		bool isRunning() const;
		size_t getClientCount() const;

		// Utility
		void cleanup();
		void logConnection(const Client& client);
		void logDisconnection(int client_fd);

	private:
		Config _config;
		Socket _serverSocket;
		std::vector<struct pollfd> _pollFds;
		std::map<int, ClientInfo> _clients;
		std::string _host;
		int _port;
		bool _running;
		int _maxClients;
};

#endif
