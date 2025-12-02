/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   connection_pool_manager.cpp                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/21 00:48:17 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/12/01 21:49:05 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "connection_pool_manager.hpp"
#include "request_router.hpp"
#include "request_handler.hpp"
#include "cgi_executor.hpp"
#include "session.hpp"
#include "session_handler.hpp"

/*
 Linux:
  - Graceful FIN: POLLHUP | POLLIN together
  - May have buffered data before FIN

 BSD/macOS:
  - Graceful FIN: Often just POLLIN, no POLLHUP
  - recv() == 0 signals FIN
*/

void ConnectionPoolManager::handleConnectionEvent(int fd, short revents, CgiExecutor& cgiExecutor) {

	Connection& connection = getConnectionRef(fd);
	ConnectionState state = connection.getState();

	// ========== POLLERR Events ==========
	if (revents & POLLERR ) {
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

		if (revents & POLLIN) {	// connection closed, but there's data in the buffer
			if (connection.getState() == READING_HEADERS) {	// client send last request and closed send, but still waiting for response.
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
				disconnectConnection(fd);
				return;
			}
		} else {	// POLLOUT
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
			if(!request.getStatus()) {
				connection.setStatusCode(400);
				connection.prepareResponse();
				return;
			}
			connection.setRequest(request);

			// Session middleware: attach session if cookie present
			std::string sid = request.getCookie("webserv_sid");
			if (!sid.empty()) {
				SessionStore& store = SessionStore::getInstance();
				Session* session = store.getSession(sid);
				if (session && !session->isExpired()) {
					connection.setSession(session);
					store.touchSession(sid);  // Update last_access
					std::cout << "[SESSION] Session found for FD " << fd << ": user_id=" << session->user_id << std::endl;
				} else {
					std::cout << "[SESSION] Invalid or expired session for FD " << fd << std::endl;
				}
			}
		}

		state = connection.getState();

		if (state == ROUTING_REQUEST) {

			// Check for session endpoints BEFORE routing
			const HttpRequest& req = connection.getRequest();
			std::string path = req.getPath();

			if (path == "/api/login" && req.getMethod() == "POST") {
				SessionHandler::handleLogin(connection);
				connection.setState(SENDING_RESPONSE);
				return;
			}
			if (path == "/api/logout" && req.getMethod() == "POST") {
				SessionHandler::handleLogout(connection);
				connection.setState(SENDING_RESPONSE);
				return;
			}

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

			// Protect /admin.html - require valid session
			if (path == "/admin.html" && req.getMethod() == "GET") {
				Session* session = connection.getSession();
				if (!session || session->user_id == 0) {
					std::cout << "[SESSION] Unauthorized access to /admin.html, redirecting to /login.html" << std::endl;
					connection.setStatusCode(303);
					connection.setRedirectUrl("/login.html");
					connection.prepareResponse();
					return;
				}
			}

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
					cgiExecutor.handleCGI(connection);
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
			if(!request.getStatus()) {
				connection.setStatusCode(400);
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
					cgiExecutor.handleCGI(connection);
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
	close(fd);
	_connectionPool.erase(fd);
}
bool ConnectionPoolManager::isConnection(int fd){
	return _connectionPool.find(fd) != _connectionPool.end();
}
