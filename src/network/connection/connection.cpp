#include "connection.hpp"
#include "post_handler.hpp"
#include "response.hpp"

Connection::Connection()
	: _fd(-1),
	_connectionState(READING_HEADERS),
	_ip(""),
	_connectionPort(0),
	_serverPort(0),
	_fileStream(NULL),
	_bytesWritten(0),
	_multipart(),
	_currentPartIndex(0),
	_bytesSent(0),
	_statusCode(0),
	_lastActivity(time(NULL)),
	_keepAliveTimeout(15),
	_maxRequests(100),
	_requestCount(0),
	_shouldClose(false) {}

Connection::Connection(int fd, const std::string& ip, int connectionPort, int serverPort)
	: _fd(fd),
	_connectionState(READING_HEADERS),
	_ip(ip),
	_connectionPort(connectionPort),
	_serverPort(serverPort),
	_fileStream(NULL),
	_bytesWritten(0),
	_multipart(),
	_currentPartIndex(0),
	_bytesSent(0),
	_statusCode(0),
	_lastActivity(time(NULL)),
	_keepAliveTimeout(15),
	_maxRequests(100),
	_requestCount(0),
	_shouldClose(false) {}

Connection::Connection(const Connection& other)
	: _fd(other._fd),
	_connectionState(other._connectionState),
	_ip(other._ip),
	_connectionPort(other._connectionPort),
	_serverPort(other._serverPort),
	_requestBuffer(other._requestBuffer),
	_request(other._request),
	_routingResult(other._routingResult),
	_fileStream(NULL),
	_uploadPath(other._uploadPath),
	_fileName(other._fileName),
	_bytesWritten(other._bytesWritten),
	_multipart(other._multipart),
	_currentPartIndex(other._currentPartIndex),
	_bodyContent(other._bodyContent),
	_responseData(other._responseData),
	_bytesSent(other._bytesSent),
	_statusCode(other._statusCode),
	_lastActivity(other._lastActivity),
	_keepAliveTimeout(other._keepAliveTimeout),
	_maxRequests(other._maxRequests),
	_requestCount(other._requestCount),
	_shouldClose(other._shouldClose) {}

Connection& Connection::operator=(const Connection& other) {
	if (this != &other) {
		// Clean up existing fileStream
		if (_fileStream) {
			if (_fileStream->is_open())
				_fileStream->close();
			delete _fileStream;
			_fileStream = NULL;
		}

		// Copy all members
		_fd = other._fd;
		_connectionState = other._connectionState;
		_ip = other._ip;
		_connectionPort = other._connectionPort;
		_serverPort = other._serverPort;
		_requestBuffer = other._requestBuffer;
		_request = other._request;
		_routingResult = other._routingResult;
		_uploadPath = other._uploadPath;
		_fileName = other._fileName;
		_bytesWritten = other._bytesWritten;
		_multipart = other._multipart;
		_currentPartIndex = other._currentPartIndex;
		_bodyContent = other._bodyContent;
		_responseData = other._responseData;
		_bytesSent = other._bytesSent;
		_statusCode = other._statusCode;
		_lastActivity = other._lastActivity;
		_keepAliveTimeout = other._keepAliveTimeout;
		_maxRequests = other._maxRequests;
		_requestCount = other._requestCount;
		_shouldClose = other._shouldClose;
		// _fileStream remains NULL (no deep copy of file streams)
	}
	return *this;
}

Connection::~Connection() {
	if (_fileStream) {
		if (_fileStream->is_open())
			_fileStream->close();
		delete _fileStream;
		_fileStream = NULL;
	}
}

bool Connection::readHeaders() {

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
		std::cout << "[DEBUG] Looking for \\r\\n\\r\\n in buffer. Found at: "
				  << headerEnd << " Buffer size: " << _requestBuffer.size() << std::endl;
  		std::cout << "[DEBUG] Buffer content: [" << _requestBuffer << "]" << std::endl;

		if(headerEnd == std::string::npos) {
			return false;  // Keep receiving headers
		}

		_connectionState = ROUTING_REQUEST;
		return true;
	}
}
bool Connection::readBody() {

	size_t contentLength = _request.getContentLength();
	size_t bodyStart = _requestBuffer.find("\r\n\r\n") + 4;
	size_t bodyReceived = _requestBuffer.length() - bodyStart;

	// Check if already complete
	if (bodyReceived >= contentLength) {
		_connectionState = EXECUTING_REQUEST;
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

/* if (_request.getTransferEncoding() == "chunked") {
		if (_request.getVersion() == "HTTP/1.0"){
			// Error: 400 Bad Request or 505 Version Not Supported
			setStatusCode(400);
			prepareResponse();
			return false;
		}
		std::string unchunkedResult = processChunkedData(buffer, bytes);
		// _requestBuffer.append(unchunkedResult);
		// ❌ Missing: check if final chunk (0\r\n\r\n) received
		// ❌ No state transition logic
	}
	else if (contentLength > 0) {
		// Read fixed-size body (both HTTP/1.0 and HTTP/1.1)
		readFixedBody(contentLength);
	}
	else {
		//no body
	} */

	updateClientActivity();
	_requestBuffer.append(buffer, bytes);

	bodyReceived = _requestBuffer.length() - bodyStart;
	std::cout << "[DEBUG] Content-Length: " << contentLength << ", Body received: " << bodyReceived << ", Total data: " << _requestBuffer.length() << std::endl;

	// Check if full body received
	if(bodyReceived < contentLength)
		return false;  // Keep receiving body
	else {
		_connectionState = EXECUTING_REQUEST;
		return true;
	}

}

bool Connection::writeOnDisc() {

	if(_multipart.empty()) {
		std::string filePath = _uploadPath + _fileName;
		if (processWriteChunck(_request.getBody(), filePath)) {
			if (static_cast<size_t>(_bytesWritten) >= _request.getBody().length()) {
				bool success = _fileStream && _fileStream->good();
				_fileStream->close();
				delete _fileStream;
				_fileStream = NULL;
				setStatusCode(success ? 200 : 500);
				prepareResponse();
				return true;
			}
		}
		return false;
	} else {

		if (static_cast<size_t>(_currentPartIndex) >= _multipart.size()) {
			setStatusCode(200);
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
			if (static_cast<size_t>(_bytesWritten) >= part.content.length()) {
				_fileStream->close();
				delete _fileStream;
				_fileStream = NULL;
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

	if (!_fileStream || !_fileStream->is_open()) {
		if (!_fileStream)
			_fileStream = new std::ofstream();
		_fileStream->open(filePath.c_str(), std::ios::binary);
		if (!_fileStream->is_open()) {
			setStatusCode(500);
			prepareResponse();
			return false;
		}
	}

	const char* writeData = data.c_str() + _bytesWritten;
	size_t remainingLen = data.length() - _bytesWritten;
	size_t bytesToWrite = std::min(remainingLen, static_cast<size_t>(BUFFER_SIZE_32));
	_fileStream->write(writeData, bytesToWrite);

	if (_fileStream->fail()) {
		std::cout << "[ERROR] File write failed" << std::endl;
		_fileStream->close();
		delete _fileStream;
		_fileStream = NULL;
		setStatusCode(500);
		prepareResponse();
		return false;
	}

	_bytesWritten += bytesToWrite;
	updateClientActivity();
	std::cout << "[DEBUG] Wrote " << bytesToWrite << " bytes (" << _bytesWritten << "/" << data.length() << ")" << std::endl;
	return true;
}
/* bool Connection::processChunkedData() {
	while (true) {
		if (_chunkState == READING_SIZE) {
			size_t crlf = _requestData.find("\r\n", bodyStart);
			if (crlf == npos) return false; // Need more data

			std::string sizeStr = _requestData.substr(bodyStart, crlf - bodyStart);
			_currentChunkSize = strtoul(sizeStr.c_str(), NULL, 16); // Hex

			if (_currentChunkSize == 0) {
				_connectionState = EXECUTING_REQUEST;
				return true; // Done
			}
			bodyStart = crlf + 2;
			_chunkState = READING_DATA;
		}

		if (_chunkState == READING_DATA) {
			if (_requestData.length() - bodyStart < _currentChunkSize + 2)
				return false; // Need more data

			_dechunkedBody.append(_requestData, bodyStart, _currentChunkSize);
			bodyStart += _currentChunkSize + 2; // Skip data + \r\n
			_chunkState = READING_SIZE;
		}
	}
} */

bool Connection::prepareResponse() {

	HttpResponse response;

	// Set response metadata
	response.setMethod(_routingResult.type);
	response.setConnectionType(_request.getConnectionType());
	response.setProtocolVersion(_request.getProtocolVersion());

	// Set body for GET request
	if (!_bodyContent.empty()) {
		response.setBody(_bodyContent);
		_bodyContent.clear();
	}

	// Set path for MIME type detection (for GET)
	if (!_routingResult.mappedPath.empty()) {
		response.setPath(_routingResult.mappedPath);
	}

	// Look up custom error page if status is an error
	if (_statusCode >= 400 && _routingResult.serverConfig){
		setupErrorPage(response);
		_shouldClose = true;
	}

	// Override connection type to close if needed
	if (_shouldClose || _request.getConnectionType() == "close" || _requestCount + 1 >= _maxRequests){
		response.setConnectionType("close");
		_shouldClose = true;
	}

	// Add cookies to response
	for (size_t i = 0; i < _responseCookies.size(); ++i) {
		response.setCookie(_responseCookies[i].name, _responseCookies[i].value,
		                  _responseCookies[i].maxAge, _responseCookies[i].path,
		                  _responseCookies[i].httpOnly, _responseCookies[i].secure);
	}

	response.generateResponse(_statusCode);
	_responseData = response.getResponse();
	_bytesSent = 0;
	_connectionState = SENDING_RESPONSE;
	return true;
}
bool Connection::prepareResponse(const std::string& cgiOutput){

	HttpResponse response;

	// Set response metadata
	response.setMethod(_routingResult.type);
	response.setConnectionType(_request.getConnectionType());
	response.setProtocolVersion(_request.getProtocolVersion());

	// Look up custom error page if status is an error
	if (_statusCode >= 400 && _routingResult.serverConfig){
		setupErrorPage(response);
		_shouldClose = true;
	}

	// Override connection type to close if needed
	if (_shouldClose || _request.getConnectionType() == "close" || _requestCount + 1 >= _maxRequests){
		response.setConnectionType("close");
		_shouldClose = true;
	}

	// Add cookies to response
	for (size_t i = 0; i < _responseCookies.size(); ++i) {
		response.setCookie(_responseCookies[i].name, _responseCookies[i].value,
							_responseCookies[i].maxAge, _responseCookies[i].path,
							_responseCookies[i].httpOnly, _responseCookies[i].secure);
	}

	response.generateResponse(_statusCode, cgiOutput);
	_responseData = response.getResponse();
	_bytesSent = 0;
	_connectionState = SENDING_RESPONSE;
	return true;

}
void Connection::setupErrorPage(HttpResponse& response) {
	std::string errorPage = _routingResult.serverConfig->getErrorPage(
		_statusCode,
		_routingResult.location
	);

	if (!errorPage.empty()) {
		response.setCustomErrorPage(errorPage);
	}
}

bool Connection::sendResponse() {

	std::cout << "[DEBUG] POLLOUT event on client FD " << _fd << " (sending response)" << std::endl;

	// Send remaining response data
	const char* data = _responseData.c_str() + _bytesSent;
	size_t remainingLean = _responseData.length() - _bytesSent;
	size_t bytesToWrite = std::min(remainingLean, static_cast<size_t>(BUFFER_SIZE_32));

	int bytes_sent = send(_fd, data, bytesToWrite, 0);
	std::cout << "[DEBUG] send() returned " << bytes_sent << " bytes to FD " << _fd << std::endl;

	if (bytes_sent > 0) {

		_bytesSent += bytes_sent;

		updateClientActivity();

		std::cout << "[DEBUG] Total bytes setn: " << _bytesSent << "    ResponseData length: " << _responseData.length() << std::endl;

		// Check if entire response was sent
		if (_bytesSent == _responseData.length()) {

			if(_shouldClose) {
				std::cout << "[DEBUG] Complete response sent to FD " << _fd << ". Closing connection." << std::endl;
				return true;
			}

			std::cout << "[DEBUG] Complete response sent to FD " << _fd << ". Resetting for next request (keep-alive)." << std::endl;
			resetForNextRequest(); // Reset client state for next request

			return false;

		} else {
			std::cout << "[DEBUG] Partial send: " << _bytesSent << "/" << _responseData.length() << " bytes sent" << std::endl;
			return false;
		}
	} else {
		// Send failed, close connection
		std::cout << "[DEBUG] Send failed for FD " << _fd << ". Closing connection." << std::endl;
		return false;
	}
}
void Connection::resetForNextRequest() {

	// Request data
	_request = HttpRequest();
	_requestBuffer.clear();
	_routingResult = RoutingResult();

	// File upload
	if (_fileStream) {
		if (_fileStream->is_open())
			_fileStream->close();
		delete _fileStream;
		_fileStream = NULL;
	}
	_uploadPath.clear();
	_fileName.clear();
	_bytesWritten = 0;
	_multipart.clear();
	_currentPartIndex = 0;

	// Response data
	_bodyContent.clear();
	_responseData.clear();
	_bytesSent = 0;
	_statusCode = 0;

	// Clear cookies
	_responseCookies.clear();

	// State
	_connectionState = READING_HEADERS;
	_requestCount++;
}

void Connection::updateClientActivity() {

	_lastActivity = time(NULL);
}
void Connection::updateKeepAliveSettings(int keepAliveTimeout, int maxRequests) {
	_keepAliveTimeout = keepAliveTimeout;
	_maxRequests = maxRequests;
}

void Connection::addCookie(const std::string& name, const std::string& value,
                          int maxAge, const std::string& path,
                          bool httpOnly, bool secure) {
	CookieData cookie;
	cookie.name = name;
	cookie.value = value;
	cookie.maxAge = maxAge;
	cookie.path = path;
	cookie.httpOnly = httpOnly;
	cookie.secure = secure;
	_responseCookies.push_back(cookie);
}

/* bool isRequestComplete() {} */

