
#ifndef CLIENT_INFO
#define CLIENT_INFO

#include <string>
#include <socket.hpp>

// Client connection states
enum ClientState {
	READING_REQUEST,   // Waiting to read HTTP request
	SENDING_RESPONSE,   // Ready to send HTTP response
	WAITING_CGI
};


// Structure to track client connection info
struct ClientInfo {

	ClientInfo()
		: socket(),
		  state(READING_REQUEST),
		  bytesSent(0),
		  shouldClose(false) {}

	ClientInfo(int fd)
		: socket(fd),
		  state(READING_REQUEST),
		  bytesSent(0),
		  shouldClose(false) {}

	//connection data
	int			fd;
	std::string	ip;
	int			port;

	//state
	ClientState	state;
	size_t		bytesSent;
	std::string	requestData;
	std::string	responseData;

	//timeout data
	time_t		lastActivity;        // Last time client sent data
	int			keepAliveTimeout;       // Timeout in seconds (default 15)

	//request limits
	int			maxRequests;            // Max requests per connection
	int			requestCount;           // Current request count

	//flag to send data
	bool		shouldClose;           // Close on error
};

#endif
