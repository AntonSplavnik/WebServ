#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "http_response.hpp"
#include "post_handler.hpp"

class RequestHandler {

	public:
		RequestHandler();
		~RequestHandler();

		void handleCGI(const HttpRequest& request, ClientInfo& client, std::string mappedPath);
		void handleGET(const HttpRequest& request, ClientInfo& client, std::string mappedPath);
		void handlePOST(const HttpRequest& request, ClientInfo& client, std::string mappedPath);
		void handleDELETE(const HttpRequest& request, ClientInfo& client, std::string mappedPath);

	private:
		bool validateMethod(const HttpRequest& request, const LocationConfig*& location);
		std::string mapPath(const HttpRequest& request, const LocationConfig*& matchedLocation);
		bool isPathSafe(const std::string& mappedPath, const std::string& allowedRoot);

};


class RequestHandler {

	public:
		void handleStatic(Connection* conn, const ServerConfig* config, const LocationConfig* location);
		void handleUpload(Connection* conn, const ServerConfig* config, const LocationConfig* location);
		void handleDelete(Connection* conn, const ServerConfig* config, const LocationConfig* location);
		void handleRedirect(Connection* conn, const LocationConfig* location);
		void handleError(Connection* conn, int statusCode, const ServerConfig* config);

	private:
		std::string mapPath(const HttpRequest& req, const LocationConfig* location);
		bool validateMethod(const HttpRequest& req, const LocationConfig* location);
		bool isPathSafe(const std::string& path, const std::string& root);
};

#endif
