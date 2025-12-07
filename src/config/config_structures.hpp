#ifndef CONFIG_STRUCTURES_HPP
#define CONFIG_STRUCTURES_HPP

#include <string>
#include <map>
#include <vector>

// Constants
#define SERVER_SOFTWARE_NAME "Webserv42/1.0"
const int MAX_BACKLOG = 1024;
const size_t MAX_CLIENTS = 1024;
const size_t MAX_CLIENT_BODY_SIZE = 1024 * 1024 * 1024; // 1GB

// Default error pages
#define DEFAULT_ERROR_PAGE_400 "runtime/www/errors/400.html"
#define DEFAULT_ERROR_PAGE_403 "runtime/www/errors/403.html"
#define DEFAULT_ERROR_PAGE_404 "runtime/www/errors/404.html"
#define DEFAULT_ERROR_PAGE_405 "runtime/www/errors/405.html"
#define DEFAULT_ERROR_PAGE_500 "runtime/www/errors/500.html"
#define DEFAULT_ERROR_PAGE_502 "runtime/www/errors/502.html"
#define DEFAULT_ERROR_PAGE_503 "runtime/www/errors/503.html"

// Location configuration
struct LocationConfig {
	LocationConfig();

	// URL matching
	std::string path;

	// File serving
	bool autoindex;
	std::string index;
	std::string root;

	// HTTP behavior
	std::vector<std::string> allow_methods;
	std::map<int, std::string> error_pages;
	unsigned long client_max_body_size;

	// CGI configuration
	std::vector<std::string> cgi_path;
	std::vector<std::string> cgi_ext;

	// File uploads
	bool upload_enabled;
	std::string upload_store;

	// Redirects
	std::string redirect;
	int redirect_code;
};

// Server configuration
struct ConfigData {
	ConfigData();

	// Find location that matches the given request path
	const LocationConfig* findMatchingLocation(const std::string& requestPath) const;

	// Get error page path for given status code
	std::string getErrorPage(int statusCode, const LocationConfig* location) const;

	// Network binding
	std::vector<std::pair<std::string, unsigned short> > listeners;
	std::vector<std::string> server_names;

	// File serving
	std::string root;
	std::string index;
	bool autoindex;

	// Network configuration
	int backlog;

	// Keep-Alive configuration
	int keepalive_timeout;
	int keepalive_max_requests;

	// HTTP behavior
	std::vector<std::string> allow_methods;
	std::map<int, std::string> error_pages;
	unsigned long client_max_body_size;

	// CGI configuration
	std::vector<std::string> cgi_path;
	std::vector<std::string> cgi_ext;

	// Location blocks
	std::vector<LocationConfig> locations;
};

#endif
