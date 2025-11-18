#include "connection.hpp"

Connection::Connection(int fd, const std::string& ip, int port, int serverPort)
	: _fd(fd),
	  _ip(ip),
	  _connectionPort(port),
	  _serverPort(serverPort),
	  _keepAliveTimeout(15),
	  _maxRequests(100),
	  _requestCount(0),
	  _connectionState(READING_HEADERS),
	  _bytesSent(0),
	  _lastActivity(time(NULL)),
	  _shouldClose(false) {}
Connection::~Connection() {}

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

	if(_connectionState == READING_HEADERS) {

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

	if(_connectionState == READING_BODY) {

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

}

bool Connection::writeOnDisc() {

	std::string body = _request.getBody();
	const char* data = body.c_str() + _bytesWritten;
	size_t remainingLean = body.length() - _bytesWritten;
	size_t bytesToWrite = std::min(remainingLean, static_cast<size_t>(BUFFER_SIZE_32));

	int bytesWritten = write(_fd, data, bytesToWrite);
	std::cout << "write() returned " << bytesWritten << " bytes to FD " << _fd << std::endl;

	if (bytesWritten > 0) {

		_bytesWritten += bytesWritten;

		updateClientActivity();

		std::cout << "Bytes setn: " << _bytesSent << "    ResponseData length: " << body.length() << std::endl;

		// Check if disc writeing is complete
		if (_bytesWritten == body.length()) {

			// Reset connection state for execution
			_connectionState = PREPARING_RESPONSE;
			return true;

		} else {
			std::cout << "[DEBUG] Partial write: " << _bytesWritten << "/" << body.length() << " bytes sent" << std::endl;
		}
	} else {

		// Send failed, close connection
		std::cout << "[DEBUG] Send failed for FD " << _fd << ". Closing connection." << std::endl;
		return false;
	}
}

bool Connection::prepareResponse() {

	HttpResponse response(_requestData);
	response.generateResponse(_statusCode);
	_responseData = response.getResponse();
	_bytesSent = 0;
	_connectionState = SENDING_RESPONSE;
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
				_connectionState = READING_BODY;
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

void Connection::updateKeepAliveSettings(int keepAliveTimeout, int maxRequests) {
	_keepAliveTimeout = keepAliveTimeout;
	_maxRequests = maxRequests;
}
