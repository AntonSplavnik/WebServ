#ifndef REQUEST_ROUTER_HPP
#define REQUEST_ROUTER_HPP

#include <iostream>

#include "http_request.hpp"
#include "config.hpp"

struct ServerConfig {
	// Your existing ConfigData
	std::vector<std::string> server_names;
	std::vector<std::pair<std::string, int>> listeners;
	std::string root;
	std::vector<LocationConfig> locations;
	// ... all config fields

	const LocationConfig* findLocation(const std::string& path);
};

enum RequestType{
	STATIC_FILE, // GET
	CGI_SCRIPT,  // CGI GET
	UPLOAD,      // POST, CGI POST
	DELETE       // DELETE
};

class RequestRouter {

	public:
		RequestRouter();
		~RequestRouter();




	// Find location within server
	const LocationConfig* routeToLocation(const HttpRequest& req, const ServerConfig* server);
	ConfigData* findServerConfigByPort();

	// Route by Host header to ServerConfig
	ServerConfig* routeToServer(const HttpRequest& req, ServerConfig* defaultConfig);

	// Determine request type
	RequestType classify(const HttpRequest& req, const LocationConfig* location);
	// Returns: STATIC_FILE, CGI_SCRIPT, UPLOAD, DELETE

	bool validateMethod(const HttpRequest& request, const LocationConfig*& location);
	std::string mapPath(const HttpRequest& request, const LocationConfig*& matchedLocation);
	bool validatePathSecurity(const std::string& mappedPath, const std::string& allowedRoot);


	private:
		std::vector<ServerConfig*> _configs;
};

#endif
