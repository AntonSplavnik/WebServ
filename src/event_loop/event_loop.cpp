/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   event_loop.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/06 13:19:56 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/12/02 00:17:27 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <csignal>
#include <cerrno>
#include <set>

#include "event_loop.hpp"


extern volatile sig_atomic_t g_shutdown;

void EventLoop::stop() {

	_pollFds.clear();
	_configs.clear();
	_killedPids.clear();
}

void EventLoop::rebuildPollFds() {

	if (_pollFds.size() > _listeningSocketCount)
		_pollFds.resize(_listeningSocketCount);

	std::map<int, Connection>& conPool = _connectionPoolManager.getConnectionPool();
	std::map<int, Connection>::iterator it = conPool.begin();
	for(;it != conPool.end(); ++it) {

		struct pollfd connection;
		connection.fd = it->first;
		connection.revents = 0;

		switch (it->second.getState()) {
			case READING_HEADERS:
			case READING_BODY:
				connection.events = POLLIN;
				break;
			case SENDING_RESPONSE:
				connection.events = POLLOUT;
				break;
			case ROUTING_REQUEST:
			case EXECUTING_REQUEST:
			case WAITING_CGI:
			case WRITING_DISK:
			case READING_DISK:
				continue;
			default:
				continue;
		}
		_pollFds.push_back(connection);
		// std::cout << "[DEBUG] Added client socket FD " << connection.fd << " to poll vector at index: " << (_pollFds.size() - 1) << std::endl;
	}

	std::map<int, Cgi>& cgis = _cgiExecutor.getCGI();
	std::map<int, Cgi>::iterator cit = cgis.begin();
	for(; cit != cgis.end(); ++cit) {

		struct pollfd cgiFd;
		cgiFd.fd = cit->first;
		if(cit->first == cit->second.getInFd())
			cgiFd.events = POLLOUT;
		else if(cit->first == cit->second.getOutFd())
			cgiFd.events = POLLIN;
		cgiFd.revents = 0;
		_pollFds.push_back(cgiFd);
		// std::cout << "[DEBUG] Added CGI socket FD " << cgiFd.fd << " to poll vector at index: " << (_pollFds.size() - 1) << std::endl;
	}
}

void EventLoop::initListeningSockets() {

	_listenManager.initListeningSockets(_configs);

	std::vector<int>& listeningSockets = _listenManager.getListeningSockets();

	for (size_t j = 0; j < listeningSockets.size(); j++)
	{
		struct pollfd listeningSocket;
		listeningSocket.fd = listeningSockets[j];
		listeningSocket.events = POLLIN;
		listeningSocket.revents = 0;

		_pollFds.push_back(listeningSocket);
		std::cout << "Added listening socket FD " << listeningSocket.fd << " to poll vector at index: " << (_pollFds.size() - 1) << std::endl;
	}
	_listeningSocketCount = listeningSockets.size();
}

void EventLoop::run() {

	initListeningSockets();

	while(_running && !g_shutdown) {

		rebuildPollFds();

		/* bool hasWork = !conPool.empty() && (hasWritingDisk || hasCGI);
		int timeout = hasWork ? 10 : -1; */ //optimisation for timer on disc writes
		int ret = poll(_pollFds.data(), _pollFds.size(), 10);

		//errno != EINTR check for interrupted poll
		if (ret < 0 && errno != EINTR) {
			std::cerr << "[DEBUG] Poll failed.\n";
			break;
		}

		if (ret > 0) {
			std::cout << "[DEBUG] poll() returned " << ret << " (number of FDs with events)" << std::endl;
			for(size_t i = 0; i < _pollFds.size(); i++) {

				int fd = _pollFds[i].fd;
				int revents = _pollFds[i].revents;

				if (_pollFds[i].revents == 0) continue;

				std::cout << "[DEBUG] Handling FD " << fd << " at index " << i << " with revents=" << revents << std::endl;

				if (_listenManager.isListening(fd))
					_listenManager.handleListenEvent(fd, revents, _connectionPoolManager);
				else if (_cgiExecutor.isCGI(fd))
					_cgiExecutor.handleCGIevent(fd, revents, _connectionPoolManager, _sessionManager);
				else if (_connectionPoolManager.isConnection(fd))
					_connectionPoolManager.handleConnectionEvent(fd, revents, _cgiExecutor, _sessionManager);
			}
		}
		checkConnectionsTimeouts();
		checkCgiTimeouts();
		processDiskWrites();
		processDiskReads();
		reapZombieProcesses();

		_sessionManager.cleanupIfNeeded();
	}
}

bool EventLoop::isConnectionTimedOut(std::map<int, Connection>& connections, int fd) {

	time_t now = time(NULL);
	return (now - connections[fd].getLastActivity() > connections[fd].getKeepAliveTimeout());
}
void EventLoop::checkConnectionsTimeouts() {

	std::map<int, Connection>& connections = _connectionPoolManager.getConnectionPool();
	std::map<int, Connection>::iterator it = connections.begin();

	// std::map<int, ClientInfo> &clients = server.getClients();
	// std::map<int, ClientInfo>::iterator it = clients.begin();

	for (; it != connections.end();){

		int currentFd = it->first;

		// Skip timeout check for connections waiting on CGI (CGI has its own timeout)
		if (it->second.getState() == WAITING_CGI) {
			++it;
			continue;
		}

		if(isConnectionTimedOut(connections, currentFd)){
			std::cout << "Client: " << currentFd << " timed out." << std::endl;

			// If headers were parsed, send 408 response before closing
			if (it->second.getState() != READING_HEADERS) {
				it->second.setStatusCode(408);
				it->second.prepareResponse();
				++it;
			} else {
				// Headers incomplete, just close connection
				int fdToDisconnect = currentFd;
				++it;
				_connectionPoolManager.disconnectConnection(fdToDisconnect);
			}
		}
		else
			++it;
	}
}

bool EventLoop::isCgiTimedOut(std::map<int, Cgi>& cgiMap, int fd) {
	time_t now = time(NULL);
	std::map<int, Cgi>::iterator cgiIt = cgiMap.find(fd);
	if (cgiIt == cgiMap.end()) return false;
	return (now - cgiIt->second.getStartTime() > CGI_TIMEOUT);
}
void EventLoop::checkCgiTimeouts() {

	std::map<int, Cgi>& cgiMap = _cgiExecutor.getCGI();
	std::map<int, Cgi>::iterator cgiIt = cgiMap.begin();
	std::set<pid_t> checkedPids;

	while (cgiIt != cgiMap.end()) {

		pid_t pid = cgiIt->second.getPid();

		// Skip if already checked (same CGI has 2 FDs in map)
		if (checkedPids.find(pid) != checkedPids.end()) {
			++cgiIt;
			continue;
		}

		checkedPids.insert(pid);

		if (isCgiTimedOut(cgiMap, cgiIt->first)) {

			std::cout << "[CGI TIMEOUT] Process pid=" << pid
					  << " exceeded " << CGI_TIMEOUT << "s — killing it." << std::endl;

			_cgiExecutor.handleCGItimeout(cgiIt->second, _connectionPoolManager);
			// Map is modified, restart
			cgiIt = cgiMap.begin();
			checkedPids.clear();
			continue;
		}
		++cgiIt;
	}
}

void EventLoop::processDiskWrites() {

	std::map<int, Connection>& conPool = _connectionPoolManager.getConnectionPool();
	std::map<int, Connection>::iterator it = conPool.begin();
	for (; it != conPool.end(); ++it) {
		if (it->second.getState() == WRITING_DISK) {
			it->second.writeOnDisc();
		}
	}
}

void EventLoop::processDiskReads() {

	std::map<int, Connection>& conPool = _connectionPoolManager.getConnectionPool();
	std::map<int, Connection>::iterator it = conPool.begin();
	for (; it != conPool.end(); ++it) {
		if (it->second.getState() == READING_DISK) {
			it->second.readFromDisc();
		}
	}
}

void EventLoop::reapZombieProcesses() {

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
	}
}
void EventLoop::addKilledPid(pid_t pid) { _killedPids.push_back(pid);}
