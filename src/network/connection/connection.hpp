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
	READING_REQUEST,    // Waiting to read HTTP request
	SENDING_RESPONSE,   // Ready to send HTTP response
	WAITING_CGI         // Waiting for CGI execution to finish
};

class Connection {

	public:
		Connection(int fd, const std::string& ip, int port, int serverPort);
		~Connection();

		// I/O operations
		bool readRequest();
		bool sendResponse();
		bool isRequestComplete();

		// Request access (stores parsed request)
		const HttpRequest& getRequest() const { return _request; }

		// Response setting (stores string, not HttpResponse object)
		void setResponseData(const std::string& responseData) {
			_responseData = responseData;
			_bytesSent = 0;
		}

		// State
		void setStete(ConnectionState state) {state = _connectionState;}
		ConnectionState getState() const;

		int getFd() const { return _fd;}
		const std::string& getIp () const { return _ip;}
		int getServerPort() const {return _serverPort;}

		// Keep-alive
		bool shouldClose() const {return _shouldClose;}
		void setShouldClose(bool close) {_shouldClose = close;}

		time_t getLastActivity() const {return _lastActivity;}
		int	getKeepAliveTimeout () const {return _keepAliveTimeout;}

		void updateKeepaliveSettings(int keepAliveTimeout, int maxRequests);

	private:
		// connection data
		int				_fd;
		ConnectionState	_connectionState;

		std::string		_ip;
		int				_port;
		int				_serverPort;

		// request data
		std::string		_requestData;
		HttpRequest		_request;

		// response data
		std::string		_responseData;
		size_t			_bytesSent;

		// timeout data
		time_t			_lastActivity;           // Last time client sent data
		int				_keepAliveTimeout;       // Timeout in seconds (default 15)

		// request limits
		int				_maxRequests;            // Max requests per connection
		int				_requestCount;           // Current request count

		// flag to send data
		bool			_shouldClose;           // Close on error

		void Connection::updateClientActivity();
};

#endif
