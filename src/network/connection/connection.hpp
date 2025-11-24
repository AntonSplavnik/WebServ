#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include <fstream>

#include "socket.hpp"
#include "http_request.hpp"
#include "request_router.hpp"

// #define BUFFER_SIZE_4 4096    // 4 KB
// // #define BUFFER_SIZE_8 8192    // 8 KB
// // #define BUFFER_SIZE_16 16384   // 16 KB
#define BUFFER_SIZE_32 32768   // 32 KB
// // #define BUFFER_SIZE_65 65536   // 64 KB

enum ConnectionState {
	READING_HEADERS,
	ROUTING_REQUEST,
	READING_BODY,
	EXECUTING_REQUEST,
	WRITING_DISK,
	SENDING_RESPONSE,
	WAITING_CGI
};

class Connection {
	public:
		// Constructor & Destructor
		Connection(int fd, const std::string& ip, int connectionPort, int serverPort);
		~Connection();

		// I/O Operations
		bool readHeaders();
		bool readBody();
		bool writeOnDisc();
		bool prepareResponse();
		bool prepareResponse(const std::string& cgiOutput); // For CGI
		bool sendResponse();

		// Connection Accessors
		int getFd() const { return _fd; }
		ConnectionState getState() const { return _connectionState; }
		void setState(ConnectionState state) { _connectionState = state; }
		const std::string& getIp() const { return _ip; }
		int getPort() const { return _connectionPort; }
		int getServerPort() const { return _serverPort; }

		// Request Accessors
		const std::string getRequestBuffer() const { return _requestBuffer; }
		const HttpRequest& getRequest() const { return _request; }
		void setRequest(HttpRequest& request) { _request = request; }
		void moveBodyToRequest() { _request.addBody(_requestBuffer); }

		// Routing Accessors
		const RoutingResult& getRoutingResult() const { return _routingResult; }
		void setRoutingResult(RoutingResult& result) { _routingResult = result; }

		// Response Accessors
		int getStatusCode() const { return _statusCode; }
		void setStatusCode(int statusCode) { _statusCode = statusCode; }
		void setResponseData(const std::string& responseData) { _responseData = responseData; }
		void setBodyContent(const std::string& content) { _bodyContent = content; }

		// File Upload - Single File
		std::string getUploadPath() const { return _uploadPath; }
		std::string getFileName() const { return _fileName; }
		void setFilePath(std::string& uploadPath, std::string& fileName) {
			_uploadPath = uploadPath;
			_fileName = fileName;
			_bytesWritten = 0;
		}

		// File Upload - Multipart
		const std::vector<MultipartPart>& getMultipart() const { return _multipart; }
		void setMultipart(std::string& path, std::vector<MultipartPart>& multipart) {
			_uploadPath = path;
			_multipart.swap(multipart);
			_bytesWritten = 0;
			_currentPartIndex = 0;
		}

		// Keep-Alive & Lifecycle
		bool shouldClose() const { return _shouldClose; }
		void setShouldClose(bool close) { _shouldClose = close; }
		time_t getLastActivity() const { return _lastActivity; }
		int getKeepAliveTimeout() const { return _keepAliveTimeout; }
		void updateKeepAliveSettings(int keepAliveTimeout, int maxRequests);

		/* bool processChunkedData(); */
	private:
		// Connection Metadata
		int				_fd;				// connection file descriptor is assigned after recv()
		ConnectionState	_connectionState;	// main states of the connection
		std::string		_ip;				// connection ip address
		int				_connectionPort;	// port of the connection side
		int				_serverPort;		// port on the server side the connectin came to

		// Request Data
		std::string		_requestBuffer;		// request is appended here while receiving request
		HttpRequest		_request;			// received and parsed request
		RoutingResult	_routingResult;		// data after routing request to a correct server block

		// File Writing - Single File
		std::ofstream	_fileStream;		// stram for disc writin. used in case of POST/ POST multipart
		std::string		_uploadPath;		// path for the POST request to upload the file
		std::string		_fileName;			// generated name for the file from the regular POST request, not multypart
		int				_bytesWritten;		// data size written on disc from POST request should be set internally 32KB

		// File Writing - Multipart
		std::vector<MultipartPart>	_multipart;	// vector with all parst of multypart POST request
		int				_currentPartIndex;		// index for tracking position when writing on sick in chunks

		// Response Data
		std::string		_bodyContent;	// body from GET for response
		std::string		_responseData;	// final response
		size_t			_bytesSent;		// data tracking for sending response should be set internally 32KB
		int				_statusCode;	// code to determine if the request was sucessful or not

		// Connection Lifecycle
		time_t			_lastActivity;
		int				_keepAliveTimeout;
		int				_maxRequests;	// max number of requests for a client
		int				_requestCount;
		bool			_shouldClose;	// only setup in case of error code

		// Private Helper Methods
		void updateClientActivity();
		bool processWriteChunck(const std::string& data, const std::string& filePath);
		void appendFormFieldToLog(const std::string& name, const std::string& value);
		void setupErrorPage(HttpResponse& response);
		void resetForNextRequest();
};

#endif

/* #define BUFFER_SIZE_4 4096    // 4 KB
// #define BUFFER_SIZE_8 8192    // 8 KB
// #define BUFFER_SIZE_16 16384   // 16 KB
#define BUFFER_SIZE_32 32768   // 32 KB
// #define BUFFER_SIZE_65 65536   // 64 KB

// Client connection states
enum ConnectionState {
	READING_HEADERS,
	ROUTING_REQUEST,	//
	READING_BODY,		// reading body of the request
	EXECUTING_REQUEST,	// POST or CGI POST
	WRITING_DISK,		// POST writes body on disk
	SENDING_RESPONSE,   // Ready to send HTTP response
	WAITING_CGI         // Waiting for CGI execution to finish
};

class Connection {

	public:
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

		// I/O operations
		bool readHeaders();
		bool readBody();
		bool writeOnDisc();
		bool prepareResponse();
		bool sendResponse();
		// bool isRequestComplete();

		const std::string getRequestBuffer() const {return _requestBuffer;}

		// Request access (stores parsed request)
		const HttpRequest& getRequest() const {return _request;}
		void setRequest(HttpRequest& request) {_request = request;}

		// Response setting (stores string, not HttpResponse object)
		void setResponseData(const std::string& responseData) { _responseData = responseData;}

		void setRoutingResult(RoutingResult& result) {_routingResult = result;}
		const RoutingResult& getRoutingResult() const {return _routingResult;}

		int getStatusCode() const {return _statusCode;}
		void setStatusCode(int statusCode) {_statusCode = statusCode;}

		// Disk writing
		std::string getUploadPath() const {return _uploadPath;}
		std::string getFileName() const {return _fileName;}
		void setFilePath(std::string& uploadPath, std::string& fileName) {
			_uploadPath = uploadPath;
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
		void setState(ConnectionState state) {_connectionState = state;}

		const std::string& getIp () const { return _ip;}
		int getServerPort() const {return _serverPort;}
		int getPort() const {return _connectionPort;}

		// Keep-alive
		bool shouldClose() const {return _shouldClose;}
		void setShouldClose(bool close) {_shouldClose = close;}

		time_t getLastActivity() const {return _lastActivity;}
		int	getKeepAliveTimeout () const {return _keepAliveTimeout;}

		void updateKeepAliveSettings(int keepAliveTimeout, int maxRequests);
		void moveBodyToRequest() { _request.addBody(_requestBuffer);
  }
	private:
		// connection data
		int							_fd;
		ConnectionState				_connectionState;

		std::string					_ip;
		int							_connectionPort;
		int							_serverPort;

		// request data
		std::string					_requestBuffer;	// Buffer
		HttpRequest					_request;		// Parsed request
		RoutingResult				_routingResult;

		// disk writing
		std::ofstream				_fileStream;
		std::string					_uploadPath;
		std::string					_fileName;
		int							_bytesWritten;

		// disk writing - mutipart
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
 */
