#include "request_handler.hpp"
#include "connection.hpp"

void RequestHandler::handleGET(Connection& connection) {

	const std::string& path = connection.getRoutingResult().mappedPath;
	Logger* logger = connection.getLogger();
	
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open()) {
		// Differentiate 404 vs 403 if needed
		int statusCode = (errno == ENOENT ? 404 : 403);
		connection.setStatusCode(errno == ENOENT ? 404 : 403);
		
		// Log error
		if (logger) {
			if (statusCode == 404) {
				logger->logError("ERROR", "File not found: " + path);
			} else {
				logger->logError("ERROR", "Permission denied: " + path + " (" + std::string(strerror(errno)) + ")");
			}
		}
		connection.prepareResponse();
		return;
	}

	// Read content
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();
	file.close();

	// Check read errors
	if (content.empty() && errno) {
		connection.setStatusCode(500);
		
		if (logger) {
			logger->logError("ERROR", "Failed to read file: " + path + " (" + std::string(strerror(errno)) + ")");
		}
		
		connection.prepareResponse();
		return;
	}

	// Success
	connection.setStatusCode(200);
	connection.setBodyContent(content);
	connection.prepareResponse();
}
void RequestHandler::handleDELETE(Connection& connection) {

	const std::string& path = connection.getRoutingResult().mappedPath;
	Logger* logger = connection.getLogger();
	
	std::ifstream file(path.c_str());
	if (file.is_open()) {
		file.close();
		if(std::remove(path.c_str()) == 0) {
			connection.setStatusCode(204);
			std::cout << "[DEBUG] Succes: 204 file deleted" << std::endl;
			
			if (logger) {
				logger->logError("INFO", "File deleted successfully: " + path);
			}
		} else {
			connection.setStatusCode(403);
			std::cout << "[DEBUG] Error: 403 permission denied" << std::endl;
			
			if (logger) {
				logger->logError("ERROR", "Failed to delete file (permission denied): " + path + " (" + std::string(strerror(errno)) + ")");
			}
		}
	} else {
		connection.setStatusCode(404);
		std::cout << "[DEBUG] Error: 404 path is not found" << std::endl;
		
		if (logger) {
			logger->logError("ERROR", "File not found for deletion: " + path);
		}
	}
	connection.prepareResponse();
}
void RequestHandler::handlePOST(Connection& connection) {

	const std::string& path = connection.getRoutingResult().mappedPath;
	Logger* logger = connection.getLogger();
	
	std::cout << "[DEBUG] UploadPath: " << path << std::endl;
	PostHandler post(path);

	const HttpRequest& request = connection.getRequest();

	std::string contentType = request.getContentType();
	std::cout << "[DEBUG] POST Content-Type: '" << contentType << "'" << std::endl;
	std::cout << "[DEBUG] Request valid: " << (request.getStatus() ? "true" : "false") << std::endl;

	if (contentType.find("multipart/form-data") != std::string::npos) {
		if(post.handleMultipart(connection)) {
			connection.setState(WRITING_DISK);
		} else {
			connection.setStatusCode(400);
			
			if (logger) {
				logger->logError("WARNING", "Invalid multipart data from " + connection.getIp());
			}
			
			connection.prepareResponse();
		}
	} else if (post.isSupportedContentType(contentType)) {
		post.handleFile(connection, contentType);
		connection.setState(WRITING_DISK);
	} else {
		std::cout << "[DEBUG] Unsupported Content-Type: " << contentType << std::endl;
		connection.setStatusCode(415);
		
		if (logger) {
			logger->logError("WARNING", "Unsupported Content-Type: " + contentType + " from " + connection.getIp());
		}
		
		connection.prepareResponse();;
	}

}
