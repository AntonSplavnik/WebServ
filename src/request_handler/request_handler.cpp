#include "request_handler.hpp"

void RequestHandler::handleGET(const HttpRequest& request, ClientInfo& client, std::string mappedPath) {

	std::ifstream file(mappedPath.c_str());

	HttpResponse response(request);
	response.setPath(mappedPath);
	if (file.is_open()){
		response.generateResponse(200);
	}
	else{
		std::cout << "Error: 404 path is not found" << std::endl;
		response.generateResponse(404);
	}
	client.responseData = response.getResponse();
	client.bytesSent = 0;
	client.state = SENDING_RESPONSE;
}
void RequestHandler::handlePOST(const HttpRequest& request, ClientInfo& client, std::string mappedPath) {

	std::cout << "[DEBUG] UploadPath: " << mappedPath << std::endl;
	PostHandler post(mappedPath);

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
void RequestHandler::handleDELETE(const HttpRequest& request, ClientInfo& client, std::string mappedPath) {

	HttpResponse response(request);
	std::ifstream file(mappedPath.c_str());
	if (file.is_open()) {
		file.close();
		if(std::remove(mappedPath.c_str()) == 0) {
			response.generateResponse(204);
			std::cout << "[DEBUG] Succes: 204 file deleted" << std::endl;
		} else {
			response.generateResponse(403);
			std::cout << "[DEBUG] Error: 403 permission denied" << std::endl;
		}
	} else {
		std::cout << "[DEBUG] Error: 404 path is not found" << std::endl;
		response.generateResponse(404);
	}
	client.responseData = response.getResponse();
	client.bytesSent = 0;
	client.state = SENDING_RESPONSE;
}
