/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:46 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/05 21:02:17 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "socket.hpp"

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

void Socket::binding(int port) {

	struct sockaddr_in	address;
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
 	address.sin_addr.s_addr = INADDR_ANY;

	if (bind(_fd, (sockaddr*)&address, sizeof(address)) < 0) {
		std::cerr << "[DEBUG] Bind failed: " << strerror(errno) << "\n";
		close(_fd);
		_fd = -1;
		return;
	}
	_port = port;
	std::cout << "[DEBUG] Bound socket FD " << _fd << " to port " << address.sin_port << std::endl;
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
