#include "request_handler.hpp"
#include "connection.hpp"

void RequestHandler::handleGET(Connection& connection) {

	const std::string& path = connection.getRoutingResult().mappedPath;
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open()) {
		// Differentiate 404 vs 403 if needed
		connection.setStatusCode(errno == ENOENT ? 404 : 403);
		connection.prepareResponse();
		return;
	}

	// Read content
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();

	// Check read errors (not EOF, which is normal)
	if (file.fail() && !file.eof()) {
		file.close();
		connection.setStatusCode(500);
		connection.prepareResponse();
		return;
	}

	file.close();

	// Success (even if content is empty - valid for 0-byte files)
	connection.setStatusCode(200);
	connection.setBodyContent(content);
	connection.prepareResponse();
}
void RequestHandler::handleDELETE(Connection& connection) {

	const std::string& path = connection.getRoutingResult().mappedPath;
	std::ifstream file(path.c_str());
	if (file.is_open()) {
		file.close();
		if(std::remove(path.c_str()) == 0) {
			connection.setStatusCode(204);
			std::cout << "[DEBUG] Succes: 204 file deleted" << std::endl;
		} else {
			connection.setStatusCode(403);
			std::cout << "[DEBUG] Error: 403 permission denied" << std::endl;
		}
	} else {
		connection.setStatusCode(404);
		std::cout << "[DEBUG] Error: 404 path is not found" << std::endl;
	}
	connection.prepareResponse();
}
void RequestHandler::handlePOST(Connection& connection) {

	const std::string& path = connection.getRoutingResult().mappedPath;
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
			connection.prepareResponse();
		}
	} else if (post.isSupportedContentType(contentType)) {
		post.handleFile(connection, contentType);
		connection.setState(WRITING_DISK);
	} else {
		std::cout << "[DEBUG] Unsupported Content-Type: " << contentType << std::endl;
		connection.setStatusCode(415);
		connection.prepareResponse();;
	}

}
