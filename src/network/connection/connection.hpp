#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include <fstream>

#include "response.hpp"
#include "socket.hpp"
#include "request.hpp"
#include "request_router.hpp"
#include "post_handler.hpp"

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
		Connection();
		Connection(int fd, const std::string& ip, int connectionPort, int serverPort);
		Connection(const Connection& other);
		Connection& operator=(const Connection& other);
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
		int getConnectionPort() const { return _connectionPort; }
		int getServerPort() const { return _serverPort; }

		// Request Accessors
		const std::string getRequestBuffer() const { return _requestBuffer; }
		const HttpRequest& getRequest() const { return _request; }
		void setRequest(HttpRequest& request) { _request = request; }
		void moveBodyToRequest() { _request.addBody(_requestBuffer); }

		// Routing Accessors
		const RoutingResult& getRoutingResult() const { return _routingResult; }
		void setRoutingResult(RoutingResult& result) { _routingResult = result; }
		void setMappedPath(const std::string& path) { _routingResult.mappedPath = path; }

		// Response Accessors
		int getStatusCode() const { return _statusCode; }
		void setStatusCode(int statusCode) { _statusCode = statusCode; }
		void setResponseData(const std::string& responseData) { _responseData = responseData; }
		void setBodyContent(const std::string& content) { _bodyContent = content; }
		void setRedirectLocation(const std::string& location) { _redirectLocation = location; }
		const std::string& getRedirectLocation() const { return _redirectLocation; }

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
		bool getShouldClose() const { return _shouldClose; }
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
		std::ofstream*	_fileStream;		// stram for disc writin. used in case of POST/ POST multipart
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
		std::string		_redirectLocation;	// redirect URL for 3xx responses

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
