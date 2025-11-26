/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   connection_pool_manager.cpp                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/21 00:48:17 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/26 01:33:00 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "connection_pool_manager.hpp"
#include "request_router.hpp"
#include "request_handler.hpp"
#include "cgi_executor.hpp"

void ConnectionPoolManager::handleConnectionEvent(int fd, short revents, CgiExecutor& cgiExecutor) {

	Connection& connection = _connectionPool[fd];
	const ConnectionState& state = connection.getState();

	// ========== POLLERR Events ==========
	if (revents & POLLERR ) {
		disconnectConnection(fd);
		return;
	}

	// ========== POLLHUP Events ==========
	else if (revents & POLLHUP) {

		if (revents & POLLIN) {
			if (connection.getState() == READING_HEADERS) {
				connection.shouldClose();
				connection.readHeaders();
			} else if (connection.getState() == READING_BODY) {
				connection.shouldClose();
				connection.readBody();
			} else {
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
			if (!connection.readHeaders()) return;

			// Parse headers after received
			HttpRequest request;
			request.parseRequestHeaders(connection.getRequestBuffer());
			if(!request.getStatus()) {
				connection.setStatusCode(400);
				connection.prepareResponse();
				return;
			}
			connection.setRequest(request);

			return;
		}
		else if (state == ROUTING_REQUEST) {

			RequestRouter router(_configs);
			RoutingResult result = router.route(connection);
			if(!result.success){
				connection.setStatusCode(result.errorCode);
				connection.prepareResponse();
				return;
			}
			connection.setRoutingResult(result);

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
					connection.readBody();
					break;

				case CGI_GET:
					cgiExecutor.handleCGI(connection);
					break;

				case REDIRECT:
					/* reqHandler.handleRedirect(connection); */
					break;

				default:
					connection.setStatusCode(500);
					connection.prepareResponse();
					// generateErrorResponse(connection, 500, "Unknown request type");
					break;
			}
			return;
		}
		else if (state == READING_BODY) {

			if(!connection.readBody()) return;

			// Parse body after received
			connection.moveBodyToRequest();
			const HttpRequest& request = connection.getRequest();
			if(!request.getStatus()) {
				connection.setStatusCode(400);
				connection.prepareResponse();
				return;
			}

			return;
		}
		else if (state == EXECUTING_REQUEST) {

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

			if (sendComplete && connection.shouldClose()) {
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

void ConnectionPoolManager::addConnection(Connection& incomingConnection) {
	int fd = incomingConnection.getFd();
	_connectionPool[fd] = incomingConnection;
}
void ConnectionPoolManager::disconnectConnection(short fd) {
	close(fd);
	_connectionPool.erase(fd);
}
bool ConnectionPoolManager::isConnection(int fd){
	return _connectionPool.find(fd) != _connectionPool.end();
}
