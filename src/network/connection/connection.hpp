#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include <fstream>

#include "socket.hpp"
#include "http_request.hpp"

#define BUFFER_SIZE_4 4096    // 4 KB
// #define BUFFER_SIZE_8 8192    // 8 KB
// #define BUFFER_SIZE_16 16384   // 16 KB
#define BUFFER_SIZE_32 32768   // 32 KB
// #define BUFFER_SIZE_65 65536   // 64 KB

// Client connection states
enum ConnectionState {
	READING_HEADERS,
	ROUTING_REQUEST,
	READING_BODY,
	EXECUTING_REQUEST,
	WRITING_DISK,
	PREPARING_RESPONSE,
	SENDING_RESPONSE,   // Ready to send HTTP response
	WAITING_CGI         // Waiting for CGI execution to finish
};

class Connection {

	public:
		Connection(int fd, const std::string& ip, int connectionPort, int serverPort);
		~Connection();

		// I/O operations
		bool readHeaders();
		bool readBody();
		bool writeOnDisc();
		bool prepareResponse();
		bool sendResponse();
		bool isRequestComplete();

		// Request access (stores parsed request)
		const HttpRequest& getRequest() const { return _request; }

		// Response setting (stores string, not HttpResponse object)
		void setResponseData(const std::string& responseData) {
			_responseData = responseData;
			_bytesSent = 0;
		}

		int getStatusCode() const {return _statusCode;}
		void setStatusCode(int statusCode) {_statusCode = statusCode;}

		// Disc writing
		std::string getUploadPath() const {return _uploadPath;}
		std::string getFileName() const {return _fileName;}
		void setFilePath(std::string& uploadPath, std::string& fileName) {
			_uploadPath == uploadPath;
			_fileName = fileName;
			_bytesWritten = 0;}

		const std::vector<MultipartPart>& getMultipart() const {return _multipart;}
		void setMultipart(std::string& path, std::vector<MultipartPart>& multipart) {
			_uploadPath = path;
			_multipart.swap(multipart);
			_bytesWritten = 0;
			_currentPartIndex = 0;}

		// Connection data
		int getFd() const { return _fd;}
		ConnectionState getState() const {return _connectionState;}
		void setState(ConnectionState state) {state = _connectionState;}

		const std::string& getIp () const { return _ip;}
		int getServerPort() const {return _serverPort;}
		int getPort() const {return _connectionPort;}

		// Keep-alive
		bool shouldClose() const {return _shouldClose;}
		void setShouldClose(bool close) {_shouldClose = close;}

		time_t getLastActivity() const {return _lastActivity;}
		int	getKeepAliveTimeout () const {return _keepAliveTimeout;}

		void updateKeepAliveSettings(int keepAliveTimeout, int maxRequests);

	private:
		// connection data
		int							_fd;
		ConnectionState				_connectionState;

		std::string					_ip;
		int							_connectionPort;
		int							_serverPort;

		// request data
		std::string					_requestData;
		HttpRequest					_request;

		// disc writing
		std::ofstream				_fileStream;
		std::string					_uploadPath;
		std::string					_fileName;
		int							_bytesWritten;

		// disc writing - mutipart
		std::vector<MultipartPart>	_multipart;
		int							_currentPartIndex;

		// response data
		std::string					_responseData;
		size_t						_bytesSent;
		int							_statusCode;

		// timeout data
		time_t						_lastActivity;           // Last time client sent data
		int							_keepAliveTimeout;       // Timeout in seconds (default 15)

		// request limits
		int							_maxRequests;            // Max requests per connection
		int							_requestCount;           // Current request count

		// flag to send data
		bool						_shouldClose;           // Close on error

		void Connection::updateClientActivity();
		bool processWriteChunck(const std::string& data, const std::string& filePath);
		void appendFormFieldToLog(const std::string& name, const std::string& value);
};

#endif
