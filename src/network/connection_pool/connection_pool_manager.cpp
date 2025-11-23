#include "connection_pool_manager.hpp"
#include "request_router.hpp"
#include "http_response.hpp"
#include "request_handler.hpp"
#include <sys/stat.h>


ConnectionPoolManager::ConnectionPoolManager() {}
ConnectionPoolManager::~ConnectionPoolManager() {}

// comment from Maksim: old code as I suppose
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
			RequestHandler handler;

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

			// Step 2.5: Check for redirect (highest priority - works for all methods)
			if (!location->redirect.empty()) {
				handler.handleRedirect(*connection, location);
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

			// Step 5: Classify request type
			RequestType type = router.classify(req, location);
			connection.setRequestType(type);

			// Step 6: Check path existence and get file info (for GET/DELETE)
			struct stat pathStat;
			bool pathExists = router.getPathInfo(mappedPath, type, &pathStat);
			
			if (!pathExists && (type == STATIC_FILE || type == DELETE)) {
				// Path doesn't exist for GET/DELETE
				generateErrorResponse(connection, errno == ENOENT ? 404 : 403, "Path not found");
				return;
			}
			
			// Step 7: Handle directory requests for GET
			if (pathExists && type == STATIC_FILE && S_ISDIR(pathStat.st_mode)) {
					// Ensure path ends with '/'
					if (mappedPath.empty() || mappedPath[mappedPath.length() - 1] != '/') {
						mappedPath += '/';
					}
					
					// Check for index file - try to serve it, let handleGET handle errors
					if (!location->index.empty()) {
						std::string fullIndexPath = mappedPath + location->index;
						handler.handleGET(*connection, location->index, location->autoindex, fullIndexPath);
						// If handleGET succeeded (status 200), return. Otherwise continue to autoindex
						if (connection->getStatusCode() == 200) {
							return;
						}
						// File doesn't exist or error - continue to autoindex check
					}
					
					// No index file or index file doesn't exist - check autoindex
					if (location->autoindex) {
						// Generate directory listing
						std::string listing = handler.generateAutoindexHTML(connection->getRequest(), mappedPath);
						connection->setStatusCode(200);
						connection->setResponseData(listing);
						connection->setState(PREPARING_RESPONSE);
						return;
					} else {
						// Autoindex off and no index - return 403
						generateErrorResponse(connection, 403, "Directory listing forbidden");
						return;
					}
				}
			}

			// Step 8: Store routing context for later use
			connection->setRoutingContext(serverConfig, location, mappedPath);

			// Step 9: Dispatch based on type
			switch (type) {

				case STATIC_FILE:
					// Regular file (not directory) - serve it
					handler.handleGET(*connection, location->index, location->autoindex, mappedPath);
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
					handlePOST(connection, mappedPath);
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

