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
#include <arpa/inet.h>

Socket::Socket():_fd(-1){ }
Socket::Socket(short fd): _fd(fd){}
Socket::~Socket(){}

void Socket::createCustom(int domain, int type, int protocol) {

	_fd = socket(domain, type, protocol);
	if(_fd < 0)
		std::cout << "[DEBUG] socket creation error" << std::endl;

	// _is_created = true;
}
void Socket::createDefault() {

	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(_fd < 0)
		std::cout << "[DEBUG] socket creation error" << std::endl;

	// _is_created = true;
}

void Socket::setReuseAddr(bool enable) {

	setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	std::cout << "[DEBUG] Set SO_REUSEADDR option on FD " << _fd << std::endl;
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
		std::cerr << "[DEBUG] Bind failed: " << strerror(errno) << "\n";
		close(_fd);
		_fd = -1;
		return;
	}

	_port = port;
	std::cout << "[DEBUG] Bound socket FD " << _fd << " to "
			<< address << ":" << port << std::endl;
}

void Socket::listening(int backlog) {

	 if (listen(_fd, backlog) < 0) {
		std::cerr << "[DEBUG] Listen failed: " << strerror(errno) << "\n";
		close(_fd);
		_fd = -1;
		return;
	}
	std::cout << "[DEBUG] Socket FD " << _fd << " is now listening (backlog: 10)" << std::endl;
}

int Socket::accepting(sockaddr_in& client_addr) {

	socklen_t client_len = sizeof(client_addr);
	int client_fd = accept(_fd, (sockaddr*)&client_addr, &client_len);
	if (client_fd < 0)
		std::cout << "[DEBUG] Client accept Error" << std::endl;
	return client_fd;
}

void Socket::setNonBlocking(int fd) {

	std::cout << "[DEBUG] Making socket FD " << fd << " non-blocking" << std::endl;
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
