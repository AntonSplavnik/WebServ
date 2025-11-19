#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "http_response.hpp"
#include "post_handler.hpp"

class RequestHandler {

	public:
		RequestHandler();
		~RequestHandler();

		void handleGET(Connection& connection, std::string& path);
		void handlePOST(Connection& connection, std::string& path);
		void handleDELETE(Connection& connection, std::string& path);

	private:

};


// class RequestHandler {

// 	public:
// 		void handleStatic(Connection* conn, const ServerConfig* config, const LocationConfig* location);
// 		void handleUpload(Connection* conn, const ServerConfig* config, const LocationConfig* location);
// 		void handleDelete(Connection* conn, const ServerConfig* config, const LocationConfig* location);
// 		void handleRedirect(Connection* conn, const LocationConfig* location);
// 		void handleError(Connection* conn, int statusCode, const ServerConfig* config);

// 	private:

// };

#endif
