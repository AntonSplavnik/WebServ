/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:46 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/17 23:58:42 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "socket.hpp"
#include "logger.hpp"
#include <arpa/inet.h>

Socket::Socket():_fd(-1){ }
Socket::Socket(short fd): _fd(fd){}
Socket::~Socket(){}

void Socket::createCustom(int domain, int type, int protocol) {

	_fd = socket(domain, type, protocol);
	if(_fd < 0)
		logError("Socket creation error");

	// _is_created = true;
}
void Socket::createDefault() {

	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(_fd < 0)
		logError("Socket creation error");

	// _is_created = true;
}

void Socket::setReuseAddr(bool enable) {

	int opt = enable ? 1 : 0;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		logError("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
		return;
	}
	logDebug("Set SO_REUSEADDR option on FD " + toString(_fd));
}

void Socket::binding(std::string address, unsigned short port) {

	struct sockaddr_in listener;
	std::memset(&listener, 0, sizeof(listener));
	listener.sin_family = AF_INET;
	listener.sin_port = htons(port);

	// Handle address conversion
	if (address.empty() || address == "0.0.0.0") {
		listener.sin_addr.s_addr = INADDR_ANY;  // Bind to all interfaces
	} else {
		if (inet_pton(AF_INET, address.c_str(), &listener.sin_addr) <= 0) {
			throw std::runtime_error("Invalid IP address: " + address);
		}
	}

	if (bind(_fd, (sockaddr*)&listener, sizeof(listener)) < 0) {
		logError("Bind failed: " + std::string(strerror(errno)));
		close(_fd);
		_fd = -1;
		return;
	}

	_port = port;
	logDebug("Bound socket FD " + toString(_fd) + " to " + address + ":" + toString(port));
}

void Socket::listening(int backlog) {

	 if (listen(_fd, backlog) < 0) {
		logError("Listen failed: " + std::string(strerror(errno)));
		close(_fd);
		_fd = -1;
		return;
	}
	logDebug("Socket FD " + toString(_fd) + " is now listening (backlog: " + toString(backlog) + ")");
}

int Socket::accepting(sockaddr_in& client_addr) {

	socklen_t client_len = sizeof(client_addr);
	int client_fd = accept(_fd, (sockaddr*)&client_addr, &client_len);
	if (client_fd < 0)
		logDebug("Client accept Error");
	return client_fd;
}

void Socket::setNonBlocking(int fd) {

	logDebug("Making socket FD " + toString(fd) + " non-blocking");
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Socket::closing(short fd) {
	if (fd >= 0) {
		::close(fd);  // Call standard close()
		fd = -1;
		// _is_bound = false;
		// _is_listening = false;
		// _is_created = false;
	}
}

int Socket::getFd() const { return _fd; }
