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

	Connection* conn = &_connectionPool[fd];

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

		// --- Phase 1: Reading headers ---
		if (conn->getState() == READING_REQUEST) {

			// Read until headers complete
			if (!conn->readRequest()) {
				return; // Headers not complete yet
			}

			// ✅ Headers complete - start routing
			const HttpRequest& req = conn->getRequest();
			RequestRouter router;

			// Step 1: Find server config by port (virtual hosting)
			ConfigData* serverConfig = findServerConfigByPort(conn->getServerPort());
			if (!serverConfig) {
				generateErrorResponse(conn, 500, "No server config found");
				return;
			}

			// Step 2: Find matching location
			const LocationConfig* location = router.findMatchingLocation(req.getPath(), serverConfig);
			if (!location) {
				generateErrorResponse(conn, 404, "Location not found");
				return;
			}

			// Step 3: Validate method allowed
			if (!router.validateMethod(req.getMethod(), location)) {
				generateErrorResponse(conn, 405, "Method not allowed");
				return;
			}

			// Step 4: Map and validate path
			std::string mappedPath = router.mapPath(req.getPath(), location);
			if (!router.validatePathSecurity(mappedPath, location->root)) {
				generateErrorResponse(conn, 403, "Path traversal detected");
				return;
			}

			// Step 5: Store routing context for later use
			conn->setRoutingContext(serverConfig, location, mappedPath);

			// Step 6: Classify request type
			RequestType type = router.classify(req, location);

			// Step 7: Dispatch based on type
			switch (type) {

				case STATIC_FILE:
					handleGET(conn, serverConfig, location, mappedPath);
					break;

				case DELETE_FILE:
					handleDELETE(conn, serverConfig, location, mappedPath);
					break;

				case POST_UPLOAD: {
					// Initialize streaming
					std::string uploadPath = location->upload_path;
					if (uploadPath.empty()) {
						uploadPath = location->root; // Fallback
					}

					std::string tempPath = uploadPath + "/temp_" + generateUniqueId() + ".raw";

					bool complete = conn->initBodyStream(tempPath);

					if (complete) {
						// Small upload completed immediately
						handlePOST(conn, serverConfig, location);
					}
					// else: state changed to STREAMING_BODY, wait for next POLLIN
					break;
				}

				case CGI_SCRIPT:
					handleCGI(conn, serverConfig, location, mappedPath);
					break;

				case REDIRECT:
					handleRedirect(conn, location);
					break;

				default:
					generateErrorResponse(conn, 500, "Unknown request type");
					break;
			}
		}

		// --- Phase 2: Streaming body ---
		else if (conn->getState() == STREAMING_BODY) {

			bool streamingComplete = conn->streamBody();

			if (streamingComplete) {
				// Retrieve stored routing context
				ConfigData* serverConfig = conn->getRoutedServer();
				const LocationConfig* location = conn->getRoutedLocation();

				// ✅ Now handle POST with complete body
				handlePOST(conn, serverConfig, location);
			}
			// else: keep streaming on next POLLIN event
		}
	}

	// ========== POLLOUT Events ==========
	else if (revents & POLLOUT) {

		if (conn->getState() == SENDING_RESPONSE) {

			bool sendComplete = conn->sendResponse();

			if (sendComplete && conn->shouldClose()) {
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
