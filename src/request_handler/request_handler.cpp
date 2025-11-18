#include "request_handler.hpp"
#include "connection.hpp"

RequestHandler::RequestHandler() {}
RequestHandler::~RequestHandler() {}

void RequestHandler::handleGET(Connection& connection, std::string path) {

	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open()) {
		// Differentiate 404 vs 403 if needed
		connection.setStatusCode(errno == ENOENT ? 404 : 403);
		connection.setState(PREPARING_RESPONSE);
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
		connection.setState(PREPARING_RESPONSE);
		return;
	}

	// Success
	connection.setStatusCode(200);
	connection.setResponseData(content);
	connection.setState(PREPARING_RESPONSE);
}
void RequestHandler::handleDELETE(Connection& connection, std::string path) {

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
	connection.setState(PREPARING_RESPONSE);
}

void RequestHandler::handlePOST(Connection& connection, std::string path) {

	std::cout << "[DEBUG] UploadPath: " << path << std::endl;
	PostHandler post(path);

	HttpRequest request = connection.getRequest();

	std::string contentType = request.getContentType();
	std::cout << "[DEBUG] POST Content-Type: '" << contentType << "'" << std::endl;
	std::cout << "[DEBUG] Request valid: " << (request.getStatus() ? "true" : "false") << std::endl;

	int statusCode;
	if (contentType.find("multipart/form-data") != std::string::npos) {
		statusCode = post.handleMultipart(request);
	} else if (post.isSupportedContentType(contentType)) {
		statusCode = post.handleFile(request, contentType);
	} else {
		std::cout << "[DEBUG] Unsupported Content-Type: " << contentType << std::endl;
		statusCode = 415;
	}
	HttpResponse response(request);
	response.generateResponse(statusCode);
	client.responseData = response.getResponse();
	client.bytesSent = 0;
	client.state = SENDING_RESPONSE;
}

