#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "post_handler.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <vector>
#include <algorithm>

class RequestHandler {

	public:
		RequestHandler(){}
		~RequestHandler(){}

		void handleGET(Connection& connection);
		void handlePOST(Connection& connection);
		void handleDELETE(Connection& connection);
		void handleRedirect(Connection& connection);

	private:
		// GET request helpers
		void handleDirectory(Connection& connection, const std::string& dirPath);
		void serveFile(Connection& connection, const std::string& filePath);

		// Autoindex helpers
		bool isDirectory(const std::string& path);
		std::string formatFileSize(off_t bytes);
		std::string formatDateTime(time_t timestamp);
		std::string generateDirectoryListing(const std::string& dirPath, const std::string& requestPath);
		void handleAutoindex(Connection& connection, const std::string& dirPath, bool autoindex);
};

#endif
