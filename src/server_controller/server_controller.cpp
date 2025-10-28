/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server_controller.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/06 13:19:56 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/29 00:35:12 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server_controller.hpp"
#include <csignal>
#include <cerrno>

extern volatile sig_atomic_t g_shutdown;

ServerController::ServerController(Config& config)
	:_configs(config.getServers()), _listeningSocketCount(), _running(true){}
ServerController::~ServerController() {

	stop();
}

void ServerController::stop() {

	for (size_t i = 0; i < _servers.size(); i++){
		delete(_servers[i]);
		_servers[i] = NULL;
	}
	_servers.clear();
	_pollFds.clear();
}

void ServerController::rebuildPollFds() {

	_pollFds.resize(_listeningSocketCount);

	for (size_t i = 0; i < _servers.size(); i++){

		std::map<int, ClientInfo>& clients = _servers[i]->getClients();
		std::map<int, ClientInfo>::iterator it = clients.begin();
		for(; it != clients.end(); ++it)
		{
			// Add client socket to poll array
			struct pollfd client;
			client.fd = it->first;
			client.revents = 0;
			switch (it->second.state)
			{
			case READING_REQUEST:
				client.events = POLLIN;
				break;
			case SENDING_RESPONSE:
				client.events = POLLOUT;
			case WAITING_CGI:
				continue;
			default:
				continue;
			}
			client.events = it->second.state == READING_REQUEST? POLLIN : POLLOUT;
			_pollFds.push_back(client);
			std::cout << "Added client socket FD " << client.fd << " to poll vector at index: " << (_pollFds.size() - 1) << std::endl;
		}

		std::map<int, Cgi*>& cgis = _servers[i]->getCGI();
		std::map<int, Cgi*>::iterator cit = cgis.begin();
		for (; cit != cgis.end(); ++cit) {

			struct pollfd cgiFd;
			cgiFd.fd = cit->first;

			if(cit->first == cit->second->getInFd())
				cgiFd.events = POLLOUT;
			else if(cit->first == cit->second->getOutFd())
				cgiFd.events = POLLIN;

			cgiFd.revents = 0;
			_pollFds.push_back(cgiFd);
			std::cout << "Added CGI socket FD " << cgiFd.fd << " to poll vector at index: " << (_pollFds.size() - 1) << std::endl;
		}
	}

}

void ServerController::initListeningSockets() {

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

void ServerController::addServers() {

	for (size_t i = 0; i < _configs.size(); i++)
	{
		Server* server = new Server(_configs[i], *this);
		_servers.push_back(server);
	}
}

void ServerController::run() {

	addServers();
	initListeningSockets();

	while(_running && !g_shutdown){

		rebuildPollFds();

		int ret = poll(_pollFds.data(), _pollFds.size(), -1);

		//errno != EINTR check for interrupted poll
		if (ret < 0){ //&& errno != EINTR) {
			std::cerr << "[DEBUG] Poll failed.\n";
			break;
		}
		std::cout << "[DEBUG] poll() completed." << std::endl;
		if (ret > 0){
			std::cout << "[DEBUG] poll() returned " << ret << " (number of FDs with events)" << std::endl;
			for(size_t i = 0; i < _pollFds.size(); i++){

				if (_pollFds[i].revents == 0)
				continue;

				std::cout << "[DEBUG] Handling FD " << _pollFds[i].fd << " at index " << i << " with revents=" << _pollFds[i].revents << std::endl;

				int fd = _pollFds[i].fd;
				short revent = _pollFds[i].revents;

				Server* srv = findServerForFd(fd);
				if (srv) srv->handleEvent(fd, revent);
			}
		}

		for (size_t i = 0; i < _servers.size(); i++)
		{
			checkClientTimeouts(*(_servers[i]));
			checkCgiTimeouts(*(_servers[i]));
		}
		reapZombieProcesses();
	}
}

Server* ServerController::findServerForFd(int fd) {

	for(size_t si = 0; si < _servers.size(); ++si){

		Server* srv = _servers[si];

		const std::vector<Socket>& listeners = srv->getListeningSockets();
		for (size_t li = 0; li < listeners.size(); ++li)
		{
			if(listeners[li].getFd() == fd) return srv;
		}

		std::map<int, ClientInfo>& clients = srv->getClients();
		if (clients.find(fd) != clients.end()) return srv;

		std::map<int, Cgi*>& cgis = srv->getCGI();
		if (cgis.find(fd) != cgis.end()) return srv;
	}
	return NULL;
}

bool ServerController::isClientTimedOut(std::map<int, ClientInfo>& clients, int fd) {

	time_t now = time(NULL);
	return (now - clients[fd].lastActivity > clients[fd].keepAliveTimeout) ;
}
void ServerController::checkClientTimeouts(Server& server) {

	std::map<int, ClientInfo>& clients = server.getClients();
	std::map<int, ClientInfo>::iterator it = clients.begin();

	for (; it != clients.end();){

		int currentFd = it->first;
		if(isClientTimedOut(clients, currentFd)){
			std::cout << "Client: " << currentFd << " timed out." << std::endl;
			int fdToDisconnect = currentFd;
			++it;
			server.disconnectClient(fdToDisconnect);
		}
		else
			++it;
	}
}

bool ServerController::isCgiTimedOut(std::map<int, Cgi*>& cgiMap, int fd) {
	time_t now = time(NULL);
	std::map<int, Cgi*>::iterator cgiIt = cgiMap.find(fd);
	if (cgiIt == cgiMap.end() || !cgiIt->second)
		return false;
	return (now - cgiIt->second->getStartTime() > CGI_TIMEOUT);
}
void ServerController::checkCgiTimeouts(Server& server) {

	std::map<int, Cgi*>& cgiMap = server.getCGI();
	std::map<int, Cgi*>::iterator cgiIt = cgiMap.begin();

	while (cgiIt != cgiMap.end()) {

		if (isCgiTimedOut(cgiMap, cgiIt->first)) {

			std::cout << "[CGI TIMEOUT] Process pid=" << cgiIt->second->getPid()
					  << " exceeded " << CGI_TIMEOUT << "s — killing it." << std::endl;

			server.handleCGItimeout(cgiIt->second);
			cgiIt = cgiMap.begin();
			continue;
		}
		++cgiIt;
	}
}

void ServerController::reapZombieProcesses() {

	if (_killedPids.empty()) return;

	for(size_t i = 0; i < _killedPids.size();) {
		int result = waitpid(_killedPids[i], NULL, WNOHANG);
		int pid = _killedPids[i];
		if (result > 0 ){
			std::cout << "[DEBUG] Reaped zombie process " << pid << std::endl;
			_killedPids.erase(_killedPids.begin() + i);
		} else if (result == 0) {
			i++;
		} else {
			std::cerr << "[WARNING] waitpid failed for pid " << pid << std::endl;
			_killedPids.erase(_killedPids.begin() + i);
		}
		//do we need i++ here?
	}
}
void ServerController::addKilledPid(pid_t pid) { _killedPids.push_back(pid);}
