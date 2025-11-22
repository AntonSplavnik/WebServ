/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   connection_pool_manager.cpp                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/21 00:48:17 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/22 22:18:54 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "connection_pool_manager.hpp"
#include "request_router.hpp"
#include "request_handler.hpp"
#include "cgi_executor.hpp"

void ConnectionPoolManager::handleConnectionEvent(int fd, short revents) {

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
			if (!connection.readHeaders()) {
				return;
			}
		}
		else if (state == ROUTING_REQUEST) {

			HttpRequest request;
			request.parseRequestHeaders(connection.getRequestBuffer());
			if(!request.getStatus()) {
				connection.setStatusCode(400);
				connection.prepareResponse();
				return;
			}
			connection.setRequest(request);

			RequestRouter router(_configs);
			RoutingResult result = router.route(connection);
			// if(!result.success){
			// 	error;
			// 	return;
			// }
			connection.setRoutingResult(result);

			RequestHandler reqHandler;
			CgiExecutor cgiExecutor;
			// Dispatch based on type
			switch (result.type) {

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
					cgiExecutor.handleCGI();
					break;

				case REDIRECT:
					reqHandler.handleRedirect(connection);
					break;

				default:
					generateErrorResponse(connection, 500, "Unknown request type");
					break;
			}
			return;
		}
		else if (state == READING_BODY) {

			if(!connection.readBody()) {
				return;
			}
		}
		else if (state == EXECUTING_REQUEST) {

			connection.moveBodyToRequest();
			const HttpRequest& request = connection.getRequest();
			if(!request.getStatus()) {
				connection.setStatusCode(400);
				connection.prepareResponse();
				return;
			}

			const RequestType& type = connection.getRoutingResult().type;
			RequestHandler reqHandler;
			CgiExecutor cgiExecutor;
			switch(type) {
				case (CGI_POST): {
					cgiExecutor.handleCGI(connection, serverConfig, location, mappedPath);
					break;
				}
				case (POST): {
					reqHandler.handlePOST(connection, mappedPath);
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

void ConnectionPoolManager::addConnection(Connection& incomingConnection) {
	_connectionPool[incomingConnection.getFd()] = incomingConnection;
}
void ConnectionPoolManager::disconnectConnection(short fd) {
	close(fd);
	_connectionPool.erase(fd);
}
bool ConnectionPoolManager::isConnection(int fd){
	return _connectionPool.find(fd) != _connectionPool.end();
}
