#include "connection.hpp"
#include "post_handler.hpp"
#include "response.hpp"
#include "../debug.hpp"

Connection::Connection()
	: _fd(-1),
	_connectionState(READING_HEADERS),
	_ip(""),
	_connectionPort(0),
	_serverPort(0),
	_currentChunkSize(0),
	_readingChunkSize(true),
	_finalChunkReceived(false),
	_fileStream(NULL),
	_bytesWritten(0),
	_multipart(),
	_currentPartIndex(0),
	_inputFileStream(NULL),
	_readFilePath(""),
	_bytesRead(0),
	_bytesSent(0),
	_statusCode(0),
	_redirectUrl(""),
	_indexPath(""),
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
	_currentChunkSize(0),
	_readingChunkSize(true),
	_finalChunkReceived(false),
	_fileStream(NULL),
	_bytesWritten(0),
	_multipart(),
	_currentPartIndex(0),
	_inputFileStream(NULL),
	_readFilePath(""),
	_bytesRead(0),
	_bytesSent(0),
	_statusCode(0),
	_redirectUrl(""),
	_indexPath(""),
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
	_chunkBuffer(other._chunkBuffer),
	_currentChunkSize(other._currentChunkSize),
	_readingChunkSize(other._readingChunkSize),
	_finalChunkReceived(other._finalChunkReceived),
	_fileStream(NULL),
	_uploadPath(other._uploadPath),
	_fileName(other._fileName),
	_bytesWritten(other._bytesWritten),
	_multipart(other._multipart),
	_currentPartIndex(other._currentPartIndex),
	_inputFileStream(NULL),
	_readFilePath(other._readFilePath),
	_bytesRead(other._bytesRead),
	_bodyContent(other._bodyContent),
	_responseData(other._responseData),
	_bytesSent(other._bytesSent),
	_statusCode(other._statusCode),
	_redirectUrl(other._redirectUrl),
	_indexPath(other._indexPath),
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

		// Clean up existing input file stream
		if (_inputFileStream) {
			if (_inputFileStream->is_open())
				_inputFileStream->close();
			delete _inputFileStream;
			_inputFileStream = NULL;
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
		_chunkBuffer = other._chunkBuffer;
		_currentChunkSize = other._currentChunkSize;
		_readingChunkSize = other._readingChunkSize;
		_finalChunkReceived = other._finalChunkReceived;
		_uploadPath = other._uploadPath;
		_fileName = other._fileName;
		_bytesWritten = other._bytesWritten;
		_multipart = other._multipart;
		_currentPartIndex = other._currentPartIndex;
		_readFilePath = other._readFilePath;
		_bytesRead = other._bytesRead;
		_bodyContent = other._bodyContent;
		_responseData = other._responseData;
		_bytesSent = other._bytesSent;
		_statusCode = other._statusCode;
		_redirectUrl = other._redirectUrl;
		_indexPath = other._indexPath;
		_lastActivity = other._lastActivity;
		_keepAliveTimeout = other._keepAliveTimeout;
		_maxRequests = other._maxRequests;
		_requestCount = other._requestCount;
		_shouldClose = other._shouldClose;
		// _fileStream and _inputFileStream remain NULL (no deep copy of file streams)
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

	if (_inputFileStream) {
		if (_inputFileStream->is_open())
			_inputFileStream->close();
		delete _inputFileStream;
		_inputFileStream = NULL;
	}
}

bool Connection::readHeaders() {

	char buffer[BUFFER_SIZE_32];
	std::memset(buffer, 0, BUFFER_SIZE_32);
	int bytes = recv(_fd, buffer, BUFFER_SIZE_32 - 1, 0);
	DEBUG_LOG("[DEBUG] recv() returned " << bytes << " bytes from FD " << _fd << std::endl);

	if (bytes <= 0) {
		if (bytes == 0) {
			DEBUG_LOG("[DEBUG] Client FD " << _fd << " disconnected before headers received." << std::endl);
		} else { // bytes < 0
			DEBUG_LOG("[DEBUG] Error on FD " << _fd << ": " << strerror(errno) << std::endl);
		}
		_shouldClose = true;
		return false;
	}
	else {

		updateClientActivity();

		_requestBuffer.append(buffer, bytes);

		// Check total header size limit
		if (_requestBuffer.size() > MAX_HEADER_SIZE) {
			std::cout << "[ERROR] Headers too large (>" << MAX_HEADER_SIZE << " bytes)" << std::endl;
			setStatusCode(400);
			prepareResponse();
			return true;
		}

		// Check if headers complete
		size_t headerEnd = _requestBuffer.find("\r\n\r\n");
		DEBUG_LOG("[DEBUG] Looking for \\r\\n\\r\\n in buffer. Found at: "
				  << headerEnd << " Buffer size: " << _requestBuffer.size() << std::endl);

		if(headerEnd == std::string::npos) {
			return false;  // Keep receiving headers
		}
		// DEBUG_LOG("[DEBUG] Buffer content: [" << _requestBuffer << "]" << std::endl);
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
	DEBUG_LOG("[DEBUG] recv() returned " << bytes << " bytes from FD " << _fd << std::endl);

	if (bytes <= 0) {
		if (bytes == 0) {
			DEBUG_LOG("[DEBUG] Client FD " << _fd << " disconnected during body read - incomplete body" << std::endl);
			setStatusCode(400);
			prepareResponse();
			return false;
		} else { // bytes < 0
			DEBUG_LOG("[DEBUG] Error on FD " << _fd << ": " << strerror(errno) << std::endl);
			_shouldClose = true;
			return false;
		}
	}

	updateClientActivity();

	if (_request.getTransferEncoding() == "chunked") {

		if (_request.getProtocolVersion() == "HTTP/1.0"){
			setStatusCode(505); // Error: 400 Bad Request or 505 Version Not Supported
			prepareResponse();
			return false;
		}

		std::string unchunkedResult = processChunkedData(buffer, bytes);
		_requestBuffer.append(unchunkedResult);

		if (_finalChunkReceived) {	// Check if done
			_connectionState = EXECUTING_REQUEST;
			return true;
		}
		return false;
	}
	else if (contentLength > 0) {

		// Read fixed-size body (both HTTP/1.0 and HTTP/1.1)
		_requestBuffer.append(buffer, bytes);
		bodyReceived = _requestBuffer.length() - bodyStart;
		DEBUG_LOG("[DEBUG] Content-Length: " << contentLength << ", Body received: "
				  << bodyReceived << ", Total data: " << _requestBuffer.length() << std::endl);

		// Check if full body received
		if(bodyReceived >= contentLength) {
			_connectionState = EXECUTING_REQUEST;
			return true; // Body fully received
		}
		return false;  // Keep receiving body
	}
	else {
		return false; // no body?
	}
}

bool Connection::writeOnDisc() {

	if(_multipart.empty()) {
		std::string filePath = _uploadPath + _fileName;
		if (processWrite(_request.getBody(), filePath)) {
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
		if (processWrite(part.content, filePath)) {
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
bool Connection::readFromDisc() {

	// Open file on first call
	if (!_inputFileStream || !_inputFileStream->is_open()) {
		if (!_inputFileStream)
			_inputFileStream = new std::ifstream();
		_inputFileStream->open(_readFilePath.c_str(), std::ios::binary);

		if (!_inputFileStream->is_open()) {
			std::cout << "[ERROR] Failed to open file for reading: " << _readFilePath << std::endl;
			delete _inputFileStream;
			_inputFileStream = NULL;
			setStatusCode(errno == ENOENT ? 404 : 403);
			prepareResponse();
			return true; // Transition to SENDING_RESPONSE with error
		}
	}

	// Read 32KB chunk
	char buffer[BUFFER_SIZE_32];
	_inputFileStream->read(buffer, BUFFER_SIZE_32);
	std::streamsize bytesRead = _inputFileStream->gcount();

	if (bytesRead > 0) {
		_bodyContent.append(buffer, bytesRead);
		_bytesRead += bytesRead;
		updateClientActivity();
		DEBUG_LOG("[DEBUG] Read " << bytesRead << " bytes from disk ("
				  << _bytesRead << " total)" << std::endl);
	}

	// Check for errors (not EOF, which is normal completion)
	if (_inputFileStream->fail() && !_inputFileStream->eof()) {
		std::cout << "[ERROR] File read failed: " << _readFilePath << std::endl;
		_inputFileStream->close();
		delete _inputFileStream;
		_inputFileStream = NULL;
		setStatusCode(500);
		prepareResponse();
		return true; // Transition to SENDING_RESPONSE with error
	}

	// Check if EOF reached (file complete)
	if (_inputFileStream->eof() || bytesRead == 0) {
		_inputFileStream->close();
		delete _inputFileStream;
		_inputFileStream = NULL;

		DEBUG_LOG("[DEBUG] File read complete: " << _bytesRead << " bytes total" << std::endl);
		setStatusCode(200);
		prepareResponse();
		return true; // Transition to SENDING_RESPONSE
	}

	return false; // Continue reading next iteration
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
bool Connection::processWrite(const std::string& data, const std::string& filePath) {

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
	DEBUG_LOG("[DEBUG] Wrote " << bytesToWrite << " bytes (" << _bytesWritten << "/" << data.length() << ")" << std::endl);
	return true;
}
std::string Connection::processChunkedData(const char* data, int dataLen) {
	_chunkBuffer.append(data, dataLen);
	std::string result;
	size_t pos = 0;

	while (pos < _chunkBuffer.length()) {
		if (_readingChunkSize) {
			size_t crlf = _chunkBuffer.find("\r\n", pos);
			if (crlf == std::string::npos)
				break;  // Size line incomplete

			std::string sizeStr = _chunkBuffer.substr(pos, crlf - pos);
			_currentChunkSize = std::strtoul(sizeStr.c_str(), NULL, 16);

			if (_currentChunkSize == 0) {
				_finalChunkReceived = true;
				_chunkBuffer.clear();
				return result;
			}

			pos = crlf + 2;
			_readingChunkSize = false;
		}

		if (!_readingChunkSize) {
			if (_chunkBuffer.length() - pos < _currentChunkSize + 2)
				break;  // Data incomplete

			result.append(_chunkBuffer, pos, _currentChunkSize);
			pos += _currentChunkSize + 2;
			_readingChunkSize = true;  // Next iteration reads size
		}
	}

	// Remove processed data from buffer
	_chunkBuffer.erase(0, pos);
	return result;
}

bool Connection::prepareResponse() {

	HttpResponse response;

	// Set response metadata
	response.setMethod(_routingResult.type);
	response.setConnectionType(_request.getConnectionType());
	response.setProtocolVersion(_request.getProtocolVersion());

	// Set body for GET request and index
	if (!_bodyContent.empty()) {
		response.setBody(_bodyContent);
		_bodyContent.clear();
	}

	// Set redirect URL for redirect responses
	if (!_redirectUrl.empty()) {
		response.setRedirectUrl(_redirectUrl);
		_redirectUrl.clear();
	}

	// Set path for MIME type detection (for GET)
	if (!_indexPath.empty()) {
		response.setPath(_indexPath);
		_indexPath.clear();
	}
	else if (!_routingResult.mappedPath.empty()) {
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

	response.generateResponse(_statusCode);
	_responseData = response.getResponse();
	// std::cout << "\nRESPONSE\n" << _responseData << "\n" << std::endl;
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
	// else response.setConnectionType(_request.getConnectionType()); ??

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

	DEBUG_LOG("[DEBUG] POLLOUT event on client FD " << _fd << " (sending response)" << std::endl);

	// Send remaining response data
	const char* data = _responseData.c_str() + _bytesSent;
	size_t remainingLean = _responseData.length() - _bytesSent;
	size_t bytesToWrite = std::min(remainingLean, static_cast<size_t>(BUFFER_SIZE_32));

	int bytes_sent = send(_fd, data, bytesToWrite, 0);
	DEBUG_LOG("[DEBUG] send() returned " << bytes_sent << " bytes to FD " << _fd << std::endl);

	if (bytes_sent > 0) {

		_bytesSent += bytes_sent;

		updateClientActivity();

		DEBUG_LOG("[DEBUG] Total bytes setn: " << _bytesSent << "    ResponseData length: " << _responseData.length() << std::endl);

		// Check if entire response was sent
		if (_bytesSent == _responseData.length()) {

			if(_shouldClose) {
				DEBUG_LOG("[DEBUG] Complete response sent to FD " << _fd << ". Closing connection." << std::endl);
				return true;
			}

			DEBUG_LOG("[DEBUG] Complete response sent to FD " << _fd << ". Resetting for next request (keep-alive)." << std::endl);
			resetForNextRequest(); // Reset client state for next request

			return false;

		} else {
			DEBUG_LOG("[DEBUG] Partial send: " << _bytesSent << "/" << _responseData.length() << " bytes sent" << std::endl);
			return false;
		}
	} else {
		// Send failed, close connection
		DEBUG_LOG("[DEBUG] Send failed for FD " << _fd << ". Closing connection." << std::endl);
		return false;
	}
}
void Connection::resetForNextRequest() {

	// Request data
	_request = HttpRequest();
	_requestBuffer.clear();
	_routingResult = RoutingResult();

	// Chunked encoding
	_chunkBuffer.clear();
	_currentChunkSize = 0;
	_readingChunkSize = true;
	_finalChunkReceived = false;

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

	// File reading (GET)
	if (_inputFileStream) {
		if (_inputFileStream->is_open())
			_inputFileStream->close();
		delete _inputFileStream;
		_inputFileStream = NULL;
	}
	_readFilePath.clear();
	_bytesRead = 0;

	// Response data
	_bodyContent.clear();
	_responseData.clear();
	_redirectUrl.clear();
	_indexPath.clear();
	_bytesSent = 0;
	_statusCode = 0;

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

/* bool isRequestComplete() {} */
