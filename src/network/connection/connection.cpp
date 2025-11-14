#include "connection.hpp"

Connection::Connection(int fd, const std::string& ip, int port, int serverPort)
	: _fd(fd),
	  _ip(ip),
	  _port(port),
	  _serverPort(serverPort),
	  _keepAliveTimeout(15),
	  _maxRequests(100),
	  _requestCount(0),
	  _connectionState(READING_REQUEST),
	  _bytesSent(0),
	  _lastActivity(time(NULL)),
	  _shouldClose(false) {}
Connection::~Connection() {}

bool Connection::readRequest() {

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

	if(_connectionState == READING_REQUEST) {

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
			{
				_requestData.append(buffer, bytes);

				// Check if headers complete
				size_t headerEnd = _requestData.find("\r\n\r\n");
				if(headerEnd == std::string::npos)
					return;  // Keep receiving headers

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
			}
			return true;
		}
	}
}
bool Connection::sendResponse() {

	if (_connectionState == SENDING_RESPONSE){

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
				_connectionState = READING_REQUEST;
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
}

bool isRequestComplete() {

}
void Connection::updateClientActivity() {

	_lastActivity = time(NULL);
}

void Connection::updateKeepaliveSettings(int keepAliveTimeout, int maxRequests) {
	_keepAliveTimeout = keepAliveTimeout;
	_maxRequests = maxRequests;
}


// In connection.cpp
bool Connection::initBodyStream(const std::string& uploadPath) {

	_uploadPath = uploadPath;

	// Open file for streaming
	_uploadFd = open(_uploadPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (_uploadFd < 0) {
		std::cerr << "[ERROR] Failed to open upload file: " << _uploadPath << std::endl;
		return false;
	}

	// Write any early body data received with headers
	if (!_earlyBodyData.empty()) {
		ssize_t written = write(_uploadFd, _earlyBodyData.c_str(), _earlyBodyData.size());
		if (written > 0) {
			_bodyBytesWritten = written;
		}
		_earlyBodyData.clear();
	}

	// Check if already complete (tiny upload)
	if (_bodyBytesWritten >= _expectedBodySize) {
		close(_uploadFd);
		_uploadFd = -1;
		return true;  // Complete
	}

	// Switch to streaming state
	_connectionState = STREAMING_BODY;
	return false;  // Need more data
}

bool Connection::streamBody() {

	if (_connectionState != STREAMING_BODY)
		return false;

	char buffer[BUFFER_SIZE_32];
	int bytes = recv(_fd, buffer, BUFFER_SIZE_32, 0);

	if (bytes <= 0) {
		if (_uploadFd >= 0) close(_uploadFd);
		return false;  // Error
	}

	updateClientActivity();

	// ✅ IMMEDIATELY write to disk (blocking is OK for 42)
	ssize_t written = write(_uploadFd, buffer, bytes);

	if (written < 0) {
		std::cerr << "[ERROR] Failed to write upload chunk" << std::endl;
		close(_uploadFd);
		return false;
	}

	_bodyBytesWritten += written;

	std::cout << "[STREAMING] Written " << _bodyBytesWritten
			<< "/" << _expectedBodySize << " bytes" << std::endl;

	// Check completion
	if (_bodyBytesWritten >= _expectedBodySize) {
		close(_uploadFd);
		_uploadFd = -1;

		std::cout << "[DEBUG] Upload complete: " << _uploadPath << std::endl;

		// ✅ Streaming complete - ready for handler to finalize
		return true;
	}

	return false;  // Continue streaming
}
