/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:52 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/30 19:28:26 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <unistd.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

// #define MAX_CON_BACKLOG 10  <==== ????

class Socket{

	public:
		Socket(short fd);
		Socket();
		~Socket();

		// Socket operations
		void createDefault();
		void createCustom(int domain, int type, int protocol);
		void binding(int port);
		void listening(int backlog);
		int accepting(sockaddr_in& client_addr);
		void closing(short fd);

		// Getters
		int getFd() const;
		// bool isBound() const;
		// bool isListening() const;

		// Setters
		void setReuseAddr(bool enable);
		void setNonBlocking(void);

	private:
		short	_fd;
		// bool _is_created;
		// bool _is_bound;
		// bool _is_listening;

};

#endif
