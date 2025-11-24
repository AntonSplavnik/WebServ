#include "connection.hpp"
#include "post_handler.hpp"
#include "response.hpp"

Connection::Connection(int fd, const std::string& ip, int connectionPort, int serverPort)
	: _fd(fd),
	_ip(ip),
	_connectionPort(connectionPort),
	_serverPort(serverPort),
	_bytesWritten(0),
	_multipart(),
	_currentPartIndex(0),
	_keepAliveTimeout(15),
	_maxRequests(100),
	_requestCount(0),
	_connectionState(READING_HEADERS),
	_bytesSent(0),
	_lastActivity(time(NULL)),
	_shouldClose(false) {}

Connection::~Connection() {
	if (_fileStream.is_open())
		_fileStream.close();
}

bool Connection::readHeaders() {

	if (_requestCount >= _maxRequests){
		std::cout << "[DEBUG] Max request count reached: " << _fd << std::endl;

		/*
			1. Graceful Closure: You should finish sending the current
			response before disconnecting
			2. Signal to Client: Send Connection: close header in the
			response before closing
			3. Not an Error: This is normal behavior, not an error condition
		*/
		return true;
	}

	char buffer[BUFFER_SIZE_32];
	std::memset(buffer, 0, BUFFER_SIZE_32);
	int bytes = recv(_fd, buffer, BUFFER_SIZE_32 - 1, 0);
	std::cout << "[DEBUG] recv() returned " << bytes << " bytes from FD " << _fd << std::endl;

	if (bytes <= 0) {
		if (bytes == 0) {
			std::cout << "[DEBUG] Client FD " << _fd << " disconnected" << std::endl;
			return false;
		} else { // bytes < 0

			std::cout << "[DEBUG] Error on FD " << _fd << ": " << strerror(errno) << std::endl;
			return false;
		}
	}
	else {

		updateClientActivity();

		_requestBuffer.append(buffer, bytes);

		// Check if headers complete
		size_t headerEnd = _requestBuffer.find("\r\n\r\n");
		if(headerEnd == std::string::npos) {
			return false;  // Keep receiving headers
		}

		_connectionState = ROUTING_REQUEST;
		return true;
	}
}
bool Connection::readBody() {

	if (_requestCount >= _maxRequests){
		std::cout << "[DEBUG] Max request count reached: " << _fd << std::endl;

		/*
			1. Graceful Closure: You should finish sending the current
			response before disconnecting
			2. Signal to Client: Send Connection: close header in the
			response before closing
			3. Not an Error: This is normal behavior, not an error condition
		*/
		return true;
	}

	char buffer[BUFFER_SIZE_32];
	std::memset(buffer, 0, BUFFER_SIZE_32);
	int bytes = recv(_fd, buffer, BUFFER_SIZE_32 - 1, 0);
	std::cout << "[DEBUG] recv() returned " << bytes << " bytes from FD " << _fd << std::endl;

	if (bytes <= 0) {
		if (bytes == 0) {
			std::cout << "[DEBUG] Client FD " << _fd << " disconnected" << std::endl;
			return false;
		} else { // bytes < 0

			std::cout << "[DEBUG] Error on FD " << _fd << ": " << strerror(errno) << std::endl;
			return false;
		}
	}
	else {

		updateClientActivity();
		_requestBuffer.append(buffer, bytes);

		size_t contentLength = _request.getContentLength();
		size_t bodyStart = _requestBuffer.find("\r\n\r\n") + 4;
		size_t bodyReceived = _requestBuffer.length() - bodyStart;
		std::cout << "[DEBUG] Content-Length: " << contentLength << ", Body received: " << bodyReceived << ", Total data: " << _requestBuffer.length() << std::endl;

		// Check if full body received
		if(bodyReceived < contentLength)
			return false;  // Keep receiving body
		else {
			_connectionState = EXECUTING_REQUEST;
			return true;
		}
	}
}

bool Connection::writeOnDisc() {

	if(_multipart.empty()) {
		std::string filePath = _uploadPath + _fileName;
		if (processWriteChunck(_request.getBody(), filePath)) {
			if (_bytesWritten >= _request.getBody().length()) {
				_fileStream.close();
				_statusCode = _fileStream.good()? 200 : 500;
				prepareResponse();
				return true;
			}
		}
		return false;
	} else {

		if (_currentPartIndex >= _multipart.size()) {
			_statusCode = 200;
			prepareResponse();
			return true;
		}

		MultipartPart& part = _multipart[_currentPartIndex];

		if (part.fileName.empty()) {
			appendFormFieldToLog(part.name, part.content);
			_currentPartIndex++;
			return false;
		}

		std::string filePath = _uploadPath + part.fileName;
		if (processWriteChunck(part.content, filePath)) {
			if (_bytesWritten >= part.content.length()) {
				_fileStream.close();
				_currentPartIndex++;
				_bytesWritten = 0;
				return false;
			}
		}
		return false;
	}
	return false;
}
void Connection::appendFormFieldToLog(const std::string& name, const std::string& value) {
	std::string logFilePath = _uploadPath;
	if (!logFilePath.empty() && logFilePath[logFilePath.size() - 1] != '/') {
		logFilePath += '/';
	}
	logFilePath += "form_data.log";

	std::ofstream file(logFilePath.c_str(), std::ios::app);
	if (file.is_open()) {
		file << "Field: " << name << " = " << value << std::endl;
		file.close();
	}
}
bool Connection::processWriteChunck(const std::string& data, const std::string& filePath) {

	if (!_fileStream.is_open()) {
		_fileStream.open(filePath.c_str(), std::ios::binary);
		if (!_fileStream.is_open()) {
			_statusCode = 500;
			prepareResponse();
			return false;
		}
	}

	const char* writeData = data.c_str() + _bytesWritten;
	size_t remainingLen = data.length() - _bytesWritten;
	size_t bytesToWrite = std::min(remainingLen, static_cast<size_t>(BUFFER_SIZE_32));
	_fileStream.write(writeData, bytesToWrite);

	if (_fileStream.fail()) {
		std::cout << "[ERROR] File write failed" << std::endl;
		_fileStream.close();
		_statusCode = 500;
		prepareResponse();
		return false;
	}

	_bytesWritten += bytesToWrite;
	updateClientActivity();
	std::cout << "[DEBUG] Wrote " << bytesToWrite << " bytes (" << _bytesWritten << "/" << data.length() << ")" << std::endl;
	return true;
}

bool Connection::prepareResponse() {

	HttpResponse response(_request);
	if (!_bodyContent.empty()) {
		response.setBody(_bodyContent);
		_bodyContent.clear();
	}

	// Set path for MIME type detection (for GET)
	if (!_routingResult.mappedPath.empty()) {
		response.setPath(_routingResult.mappedPath);
	}

	// Look up custom error page if status is an error
	setupErrorPageIfNeeded(response);

	response.generateResponse(_statusCode);
	_responseData = response.getResponse();
	_bytesSent = 0;
	_connectionState = SENDING_RESPONSE;
	return true;
}
bool Connection::prepareResponse(const std::string& cgiOutput){

	HttpResponse response(_request);

	// Look up custom error page if status is an error
	setupErrorPageIfNeeded(response);

	response.generateResponse(_statusCode, cgiOutput);
	_responseData = response.getResponse();
	_bytesSent = 0;
	_connectionState = SENDING_RESPONSE;
	return true;

}
bool Connection::sendResponse() {

	std::cout << "[DEBUG] POLLOUT event on client FD " << _fd << " (sending response)" << std::endl;

	// Send remaining response data
	const char* data = _responseData.c_str() + _bytesSent;
	size_t remainingLean = _responseData.length() - _bytesSent;
	size_t bytesToWrite = std::min(remainingLean, static_cast<size_t>(BUFFER_SIZE_32));

	int bytes_sent = send(_fd, data, bytesToWrite, 0);
	std::cout << "send() returned " << bytes_sent << " bytes to FD " << _fd << std::endl;

	if (bytes_sent > 0) {

		_bytesSent += bytes_sent;

		updateClientActivity();

		std::cout << "Bytes setn: " << _bytesSent << "    ResponseData length: " << _responseData.length() << std::endl;

		// Check if entire response was sent
		if (_bytesSent == _responseData.length()) {

			if(_shouldClose){
				std::cout << "[DEBUG] Complete response sent to FD " << _fd << ". Closing connection." << std::endl;
				return true;
			}

			// Reset client state for next request
			_connectionState = READING_HEADERS;
			_bytesSent = 0;
			_responseData.clear();
			_requestBuffer.clear();

			return true;

		} else {
			std::cout << "[DEBUG] Partial send: " << _bytesSent << "/" << _responseData.length() << " bytes sent" << std::endl;
		}
	} else {

		// Send failed, close connection
		std::cout << "[DEBUG] Send failed for FD " << _fd << ". Closing connection." << std::endl;
		return false;
	}
}

void Connection::updateClientActivity() {

	_lastActivity = time(NULL);
}
void Connection::updateKeepAliveSettings(int keepAliveTimeout, int maxRequests) {
	_keepAliveTimeout = keepAliveTimeout;
	_maxRequests = maxRequests;
}

void Connection::setupErrorPageIfNeeded(HttpResponse& response) {
	if (_statusCode >= 400 && _routingResult.serverConfig) {
		std::string errorPage = _routingResult.serverConfig->getErrorPage(
			_statusCode,
			_routingResult.location
		);
		if (!errorPage.empty()) {
			response.setCustomErrorPage(errorPage);
		}
	}
}
/* bool isRequestComplete() {} */
