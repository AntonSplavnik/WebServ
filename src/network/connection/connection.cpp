#include "connection.hpp"
#include "post_handler.hpp"

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

	if(_fileStream.is_open()) _fileStream.close();
}

bool Connection::readHeaders() {

	std::cout << "\n#######  HANDLE CLIENT READ DATA #######" << std::endl;

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

		_requestData.append(buffer, bytes);

		// Check if headers complete
		size_t headerEnd = _requestData.find("\r\n\r\n");
		if(headerEnd == std::string::npos) {
			_connectionState = ROUTING_REQUEST;
			return;  // Keep receiving headers
		}

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

		_requestData.append(buffer, bytes);

		// Check if headers complete
		size_t headerEnd = _requestData.find("\r\n\r\n");

		// Parse headers to get Content-Length
		HttpRequest tempParser;
		tempParser.ParsePartialRequest(_requestData);
		size_t contentLength = tempParser.getContentLength();

		// Check if full body received
		size_t bodyStart = headerEnd + 4;
		size_t bodyReceived = _requestData.length() - bodyStart;

		std::cout << "[DEBUG] Content-Length: " << contentLength << ", Body received: " << bodyReceived << ", Total data: " << _clients[fd].requestData.length() << std::endl;

		if(bodyReceived < contentLength)
			return false;  // Keep receiving body
		else {
			_connectionState == EXECUTING_REQUEST;
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
				_connectionState = PREPARING_RESPONSE;
				return true;
			}
		}
		return false;
	} else {

		if (_currentPartIndex >= _multipart.size()) {
			_statusCode = 200;
			_connectionState = PREPARING_RESPONSE;
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
	std::string logFile = _uploadPath;
	if (!logFile.empty() && logFile[logFile.size() - 1] != '/') {
		logFile += '/';
	}
	logFile += "form_data.log";

	std::ofstream file(logFile.c_str(), std::ios::app);
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
			_connectionState = PREPARING_RESPONSE;
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
		_connectionState = PREPARING_RESPONSE;
		return false;
	}

	_bytesWritten += bytesToWrite;
	updateClientActivity();
	std::cout << "[DEBUG] Wrote " << bytesToWrite << " bytes (" << _bytesWritten << "/" << data.length() << ")" << std::endl;
	return true;
}
bool Connection::prepareResponse() {

	HttpResponse response(_requestData);
	response.generateResponse(_statusCode);
	_responseData = response.getResponse();
	_bytesSent = 0;
	_connectionState = SENDING_RESPONSE;
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
			_requestData.clear();

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

bool isRequestComplete() {

}
void Connection::updateClientActivity() {

	_lastActivity = time(NULL);
}

void Connection::updateKeepAliveSettings(int keepAliveTimeout, int maxRequests) {
	_keepAliveTimeout = keepAliveTimeout;
	_maxRequests = maxRequests;
}
