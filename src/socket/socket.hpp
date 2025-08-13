#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <unistd.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CON_BACKLOG 10

class Socket{

	public:
		Socket();
		Socket(int domain, int type, int protocol);
		~Socket();

		// Socket operations
		void create(int domain, int type, int protocol);
		void binding(const std::string& ip, int port);
		void listening(int backlog);
		int accepting();
		void closing(int _fd);

		// Getters
		int getFd() const;
		// bool isBound() const;
		// bool isListening() const;

		// Setters
		void setReuseAddr(bool enable);
		void setNonBlocking(void);

	private:
		int _fd;
		struct sockaddr_in _address;
		// bool _is_created;
		// bool _is_bound;
		// bool _is_listening;

};

#endif
