#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "post_handler.hpp"
#include "config.hpp"

class RequestHandler {

	public:
		RequestHandler(){}
		~RequestHandler(){}

		void handleGET(Connection& connection);
		void handlePOST(Connection& connection);
		void handleDELETE(Connection& connection);
		void handleRedirect(Connection& connection);

	private:
		bool tryServeIndexFile(Connection& connection, const std::string& dirPath, const LocationConfig* location);
		void serveDirectoryListing(Connection& connection, const std::string& dirPath);
		void serveRegularFile(Connection& connection, const std::string& filePath);
};

#endif
