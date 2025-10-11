/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server_controller.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/06 13:19:56 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/11 17:02:39 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server_controller.hpp"

ServerController::ServerController(Config& config)
	:_configs(config.getServers()), _running(true){}

ServerController::~ServerController(){

	stop();
}

void ServerController::stop(){

	for (size_t i = 0; i < _servers.size(); i++){
		delete(_servers[i]);
		_servers[i] = nullptr;
	}
	_servers.clear();
	_pollFds.clear();
}

bool ServerController::isClientTimedOut(std::map<int, ClientInfo>& clients, int fd){

	time_t now = time(NULL);
	return (now - clients[fd].lastActivity > clients[fd].keepAliveTimeout) ;
}

void ServerController::checkClientTimeouts(Server& server){

	std::map<int, ClientInfo>& clients = server.getClients();
	std::map<int, ClientInfo>::iterator it = clients.begin();

	for (; it != clients.end();){

		int currentFd = it->first;
		if(isClientTimedOut(clients, currentFd)){
			std::cout << "Client: " << currentFd << " timed out." << std::endl;
			int fdToDisconnect = currentFd;
			++it;
			server.disconectClient(fdToDisconnect);
		}
		else
			++it;
	}
}

Server* ServerController::findServerForFd(int fd){

	for(size_t i = 0; i < _servers.size(); i++){

		Server* srv = _servers[i];
		std::vector<Socket> listeners = srv->getListeningSockets();
		std::map<int, ClientInfo>& clients = srv->getClients();

		for (size_t i = 0; i < listeners.size(); i++)
		{
			if(listeners[i].getFd() == fd) return srv;
		}

		std::map<int, ClientInfo>::iterator it = clients.begin();
		for(; it != clients.end(); ++it)
		{
			if(it->first == fd) return srv;
		}
	}
	return nullptr;
}

void ServerController::rebuildPollFds(){

	_pollFds.resize(_listeningSocketCount);

	for (size_t i = 0; i < _servers.size(); i++){

		std::map<int, ClientInfo>& clients = _servers[i]->getClients();
		std::map<int, ClientInfo>::iterator it = clients.begin();
		for(; it != clients.end(); ++it)
		{
			// Add client socket to poll array
			struct pollfd client;
			client.fd = it->first;
			client.events = it->second.state == READING_REQUEST? POLLIN : POLLOUT;
			client.revents = 0;
			_pollFds.push_back(client);

			std::cout << "Added client socket FD " << client.fd << " to poll vector at index: " << (_pollFds.size() - 1) << std::endl;
		}
	}
}


void ServerController::initListeningSockets(){

	for (size_t i = 0; i < _servers.size(); i++){

		const std::vector<Socket>& listeningSockets = _servers[i]->getListeningSockets();

		for (size_t j = 0; j < listeningSockets.size(); j++)
		{
			struct pollfd listeningSocket;
			listeningSocket.fd = listeningSockets[j].getFd();
			listeningSocket.events = POLLIN;
			listeningSocket.revents = 0;

			_pollFds.push_back(listeningSocket);

			_listeningSocketCount++;

			std::cout << "Added listening socket FD " << listeningSocket.fd << " to poll vector at index: " << (_pollFds.size() - 1) << std::endl;
		}
	}
}

void ServerController::addServers(){

	for (size_t i = 0; i < _configs.size(); i++)
	{
		Server* server = new Server(_configs[i]);
		_servers.push_back(server);
	}
}

void ServerController::run(){

	addServers();
	initListeningSockets();

	while(_running){

		rebuildPollFds();

		int ret = poll(_pollFds.data(), _pollFds.size(), -1);
		if (ret < 0) {
			std::cerr << "Poll failed.\n";
			break;
		}
		std::cout << "poll() returned " << ret << " (number of FDs with events)" << std::endl;

		for(size_t i = 0; i < _pollFds.size(); i++){

			if (_pollFds[i].revents == 0)
			continue;

			std::cout << "DEBUG: Handling FD " << _pollFds[i].fd << " at index " << i << " with revents=" << _pollFds[i].revents << std::endl;

			int fd = _pollFds[i].fd;
			short revent = _pollFds[i].revents;

			Server* srv = findServerForFd(fd);
			if (srv) srv->handleEvent(fd, revent);
		}

		for (size_t i = 0; i < _servers.size(); i++)
		{
			checkClientTimeouts(*(_servers[i]));
		}
	}
}
