/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   connection_pool_manager.cpp                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/21 00:48:17 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/12/05 01:55:57 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "connection_pool_manager.hpp"
#include "request_router.hpp"
#include "request_handler.hpp"
#include "cgi_executor.hpp"
#include "session_manager.hpp"

void ConnectionPoolManager::handleConnectionEvent(int fd, short revents, CgiExecutor& cgiExecutor, SessionManager& sessionManager) {
	(void)sessionManager; // TODO: Use sessionManager

	// std::cout << "[DEBUG] handleConnectionEvent FD " << fd << " revents=" << revents << std::endl;

	Connection& connection = getConnectionRef(fd);
	ConnectionState state = connection.getState();

	// ========== POLLERR Events ==========
	if (revents & POLLERR ) {
		std::cout << "[DEBUG] POLLERR detected, disconnecting FD " << fd << std::endl;
		disconnectConnection(fd);
		return;
	}

	// ========== POLLHUP Events ==========
	/*
	POLLHUP alone:
	- Socket error occurred (often combined with POLLERR)
	- Write-side closed on a socket
	- Some abnormal disconnect conditions

	POLLIN | POLLHUP together:
	- Normal TCP connection close (graceful shutdown with FIN)
	- Remote peer called close() or shutdown(SHUT_WR)
	- This is the common case for normal disconnects
	*/
	else if (revents & POLLHUP) {

		// On Linux, POLLHUP may occur without POLLIN even when data is available
		// Always attempt to read if we're in a reading state
		if (connection.getState() == READING_HEADERS) {
			connection.setShouldClose(true);
			if (!connection.readHeaders()) {	// recv() returned 0
				disconnectConnection(fd);
				return;
			}
		} else if (connection.getState() == READING_BODY) {
			connection.setShouldClose(true);
			if (!connection.readBody()) {	// recv() returned 0
				disconnectConnection(fd);
				return;
			}
		} else {
			std::cout << "[DEBUG] Client FD " << fd << " hung up" << std::endl;
			disconnectConnection(fd);
			return;
		}
	}

	// ========== POLLIN Events ==========
	else if (revents & POLLIN) {

		if (state == READING_HEADERS) {

			// Read until headers complete
			if (!connection.readHeaders()) {
				if (connection.getShouldClose()) {
					disconnectConnection(fd);
				}
				return;
			}

			// Parse headers after received
			HttpRequest request;
			request.parseRequestHeaders(connection.getRequestBuffer());
			if (!request.isValid()) {
				connection.setStatusCode(request.getStatusCode());
				connection.prepareResponse();
				return;
			}
			connection.setRequest(request);
		}

		state = connection.getState();

		if (state == ROUTING_REQUEST) {

			RequestRouter router(_configs);
			RoutingResult result = router.route(connection);
			connection.setRoutingResult(result);
			if(!result.success){
				connection.setStatusCode(result.errorCode);
				connection.prepareResponse();
				return;
			}

			int keepAliveTimeout = result.serverConfig->keepalive_timeout;
			int keepaliveMaxRequests = result.serverConfig->keepalive_max_requests;
			connection.updateKeepAliveSettings(keepAliveTimeout, keepaliveMaxRequests);

			const RequestType& type = connection.getRoutingResult().type;
			RequestHandler reqHandler;
			switch (type) { // Dispatch based on type
				case GET:
					reqHandler.handleGET(connection);
					break;
				case DELETE:
					reqHandler.handleDELETE(connection);
					break;
				case POST:
				case CGI_POST:
					connection.setState(READING_BODY);
					break;
				case CGI_GET:
					cgiExecutor.handleCGI(connection, sessionManager);
					break;
				case REDIRECT:
					reqHandler.handleRedirect(connection);
					break;
				default:
					connection.setStatusCode(500);
					connection.prepareResponse();
					break;
			}
		}

		state = connection.getState();

		if (state == READING_BODY) {

			if(!connection.readBody()) return;

			// Parse body after received
			connection.moveBodyToRequest();
			const HttpRequest& request = connection.getRequest();
			if (!request.isValid()) {
				connection.setStatusCode(request.getStatusCode());
				connection.prepareResponse();
				return;
			}
		}

		state = connection.getState();

		if (state == EXECUTING_REQUEST) {

			const RequestType& type = connection.getRoutingResult().type;
			RequestHandler reqHandler;

			switch(type) {
				case (CGI_POST): {
					cgiExecutor.handleCGI(connection, sessionManager);
					break;
				}
				case (POST): {
					reqHandler.handlePOST(connection);
					break;
				}
				default: {
					std::cout << "[DEBUG] Unknown reuest type" << std::endl;
					break;
				}
				return;
			}
		}
	}

	// ========== POLLOUT Events ==========
	else if (revents & POLLOUT) {

		if (connection.getState() == SENDING_RESPONSE) {

			bool sendComplete = connection.sendResponse();

			if (sendComplete && connection.getShouldClose()) {
				disconnectConnection(fd);
			}
		}
	}
}

/** returns NULL if connection doesn't exist! */
Connection* ConnectionPoolManager::getConnection(int fd){
	std::map<int, Connection>::iterator it = _connectionPool.find(fd);
	if(it != _connectionPool.end()) return &it->second;
	return NULL;
}
Connection& ConnectionPoolManager::getConnectionRef(int fd){
	std::map<int, Connection>::iterator it = _connectionPool.find(fd);
	return it->second;
}

void ConnectionPoolManager::addConnection(Connection& incomingConnection) {
	int fd = incomingConnection.getFd();
	_connectionPool.insert(std::make_pair(fd, incomingConnection));
}
void ConnectionPoolManager::disconnectConnection(short fd) {
	close (fd);
	_connectionPool.erase(fd);
}
bool ConnectionPoolManager::isConnection(int fd){
	return _connectionPool.find(fd) != _connectionPool.end();
}
