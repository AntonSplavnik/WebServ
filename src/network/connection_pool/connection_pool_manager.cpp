#include "connection_pool_manager.hpp"
#include "request_router.hpp"


ConnectionPoolManager::ConnectionPoolManager() {}
ConnectionPoolManager::~ConnectionPoolManager() {}

void ConnectionPoolManager::handleConnectionEvent(int fd, short revents) {

	Connection* connection = &_connectionPool[fd];

	if (revents & POLLERR || revents & POLLHUP) {
		disconnectConnection(fd);
		return;
	} if (revents & POLLIN && connection->getState() == READING_REQUEST) {
		if (!connection->readRequest()) {
			return ;
		} else {
			// call router method()
		}
	} else if (revents & POLLOUT && connection->getState() == SENDING_RESPONSE) {
		connection->sendResponse();
	}
}

void ConnectionPoolManager::handleConnectionEvent(int fd, short revents) {

	Connection* connection = &_connectionPool[fd];

	// ========== POLLERR Events ==========
	if (revents & POLLERR ) {
		disconnectConnection(fd);
		return;
	}

	// ========== POLLHUP Events ==========
	else if (revents & POLLHUP) {
		if (revents & POLLIN) handleClientRead(fd); // half closed in case client is done sending and waiting for respose: shutdown(fd, SHUT_WR); recv(fd, ...);
		std::cout << "[DEBUG] Client FD " << fd << " hung up" << std::endl;
		disconnectConnection(fd);
		return;
	}

	// ========== POLLIN Events ==========
	else if (revents & POLLIN) {

		ConnectionState state = connection->getState();

		if (state == READING_HEADERS) {

			// Read until headers complete
			if (!connection->readHeaders()) {
				return;
			}
		}

		else if (state == ROUTING_REQUEST) {


			const HttpRequest& req = connection->getRequest();
			RequestRouter router;

			// Step 1: Find server config by port (virtual hosting)
			ConfigData* serverConfig = findServerConfigByPort(connection->getServerPort());
			if (!serverConfig) {
				generateErrorResponse(connection, 500, "No server config found");
				return;
			}

			// Step 2: Find matching location
			const LocationConfig* location = router.findMatchingLocation(req.getPath(), serverConfig);
			if (!location) {
				generateErrorResponse(connection, 404, "Location not found");
				return;
			}

			// Step 3: Validate method allowed
			if (!router.validateMethod(req.getMethod(), location)) {
				generateErrorResponse(connection, 405, "Method not allowed");
				return;
			}

			// Step 4: Map and validate path
			std::string mappedPath = router.mapPath(req.getPath(), location);
			if (!router.validatePathSecurity(mappedPath, location->root)) {
				generateErrorResponse(connection, 403, "Path traversal detected");
				return;
			}

			// Step 5: Store routing context for later use
			connection->setRoutingContext(serverConfig, location, mappedPath);

			// Step 6: Classify request type
			RequestType type = router.classify(req, location);
			connection.setRequestType(type);

			// Step 7: Dispatch based on type
			switch (type) {

				case STATIC_FILE:
					handleGET(connection, serverConfig, location, mappedPath);
					break;

				case DELETE:
					handleDELETE(connection, serverConfig, location, mappedPath);
					break;

				case UPLOAD:
					connection->readBody();
					break;

				case CGI_SCRIPT:
					handleCGI(connection, serverConfig, location, mappedPath);
					break;

				case REDIRECT:
					handleRedirect(connection, location);
					break;

				default:
					generateErrorResponse(connection, 500, "Unknown request type");
					break;
			}
			return;
		}

		else if (state == READING_BODY) {

			if(!connection->readBody()) {
				return;
			}
		}

		else if (state == EXECUTING_REQUEST) {

			RequestType type = connection->getRequestType();

			switch(type) {
				case (CGI){
					handleCGI(connection, serverConfig, location, mappedPath);
					break;
				}
				case (POST) {
					handlePOST(connection, serverConfig, location);
					break;
				}
				default: {
					std::cout << "[DEBUG] Unknown reuest type" << std::endl;
					break;
				}
				return;
			}
		}

		else if (state == PREPARING_RESPONSE) {

			connection->prepareResponse();
		}
	}

	// ========== POLLOUT Events ==========
	else if (revents & POLLOUT) {

		if (connection->getState() == SENDING_RESPONSE) {

			bool sendComplete = connection->sendResponse();

			if (sendComplete && connection->shouldClose()) {
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
