#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>

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

		std::string getFilePath() {return _filePath;}
		void setFilePath(std::string filePath) {_filePath == filePath;}
		const std::vector<MultipartPart>& getMultipart() const {return _multipart;}
		void setMultipart(std::vector<MultipartPart> multipart, std::string path) {
			_multipart.swap(multipart);
			path = _filePath;}

		// Connection data
		int getFd() const { return _fd;}
		ConnectionState getState() const {return _connectionState;}
		void setState(ConnectionState state) {state = _connectionState;} // comment_Mak: we need to switch state and _connectionState

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
		std::string					_filePath;
		int							_bytesWritten;
		// mutipart
		std::vector<MultipartPart>	_multipart;
		int							_partIndex;
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
};

#endif
