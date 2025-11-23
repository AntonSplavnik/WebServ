#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "http_response.hpp"
#include "post_handler.hpp"
#include "config.hpp"

class RequestHandler {

	public:
		RequestHandler();
		~RequestHandler();

	void handleGET(Connection& connection, const std::string& indexFile, bool autoindex, std::string& path);
	void handlePOST(Connection& connection, std::string& path);
	void handleDELETE(Connection& connection, std::string& path);
	void handleRedirect(Connection& connection, const LocationConfig* location);

	public:
		std::string generateAutoindexHTML(const HttpRequest& request, const std::string& dirPath);

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
