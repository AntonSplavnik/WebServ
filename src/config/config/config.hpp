#ifndef CONFIG_HPP
#define CONFIG_HPP
#include <string>
#include <map>
#include <vector>

#define SERVER_SOFTWARE_NAME "Webserv42/1.0"
const int MAX_BACKLOG = 1024;
const size_t MAX_CLIENTS = 1024;  // Global connection limit
const size_t MAX_CLIENT_BODY_SIZE = 1024 * 1024 * 1024; // 1GB

// Valid CGI extensions
static const char *VALID_CGI_EXTENSIONS[] = {
	".cgi", ".pl", ".py", ".php", ".sh", ".rb", ".js", ".asp", ".exe", ".bat", ".tcl", ".lua", ".bla"
};
static const size_t VALID_CGI_EXTENSIONS_COUNT = sizeof(VALID_CGI_EXTENSIONS) / sizeof(VALID_CGI_EXTENSIONS[0]);

// Valid autoindex values
static const char *AUTOINDEX_VALUES[] = {"on", "off", "true", "false", "1", "0"};
static const size_t AUTOINDEX_VALUES_COUNT = sizeof(AUTOINDEX_VALUES) / sizeof(AUTOINDEX_VALUES[0]);

// Valid HTTP methods
static const char *HTTP_METHODS[] = {"GET", "POST", "DELETE"};
static const size_t HTTP_METHODS_COUNT = sizeof(HTTP_METHODS) / sizeof(HTTP_METHODS[0]);

//Valid location directives (used in config.cpp)
static const char *LOCATION_DIRECTIVES[] = {
	"autoindex", "root", "index", "allow_methods", "cgi_ext", "cgi_path",
	"upload_enabled", "upload_store", "redirect", "error_page", "client_max_body_size"
};
static const size_t LOCATION_DIRECTIVES_COUNT = sizeof(LOCATION_DIRECTIVES) / sizeof(LOCATION_DIRECTIVES[0]);

//Valid server directives (used in config.cpp)
static const char *SERVER_DIRECTIVES[] = {
	"location", "listen", "server_name", "backlog",
	"access_log", "error_log", "autoindex", "index", "root",
	"allow_methods", "error_page", "cgi_ext", "cgi_path",
	"client_max_body_size", "keepalive_timeout", "keepalive_max_requests"
};
static const size_t SERVER_DIRECTIVES_COUNT = sizeof(SERVER_DIRECTIVES) / sizeof(SERVER_DIRECTIVES[0]);

// Default error pages
#define DEFAULT_ERROR_PAGE_404 "runtime/www/errors/404.html"
#define DEFAULT_ERROR_PAGE_500 "runtime/www/errors/500.html"
#define DEFAULT_ERROR_PAGE_403 "runtime/www/errors/403.html"
#define DEFAULT_ERROR_PAGE_413 "runtime/www/errors/400.html"


struct LocationConfig
{
	LocationConfig();

	// URL matching
	std::string path;

	// File serving
	bool autoindex;
	std::string index; // default_index
	std::string root; // document_root

	// HTTP behavior
	std::vector<std::string> allow_methods;
	std::map<int, std::string> error_pages;
	int client_max_body_size;

	// CGI configuration
	std::vector<std::string> cgi_path; // cgi_interpreters
	std::vector<std::string> cgi_ext; // cgi_extensions


	// File uploads
	bool upload_enabled;
	std::string upload_store; // upload_directory

	// Redirects
	std::string redirect; // redirect_url
	int redirect_code; // redirect_status_code
};

struct ConfigData
{
	ConfigData();

	// Find location that matches the given request path
	const LocationConfig* findMatchingLocation(const std::string& requestPath) const;

	// Get error page path for given status code
	std::string getErrorPage(int statusCode, const LocationConfig* location) const;

	// Network binding
	std::vector<std::pair<std::string, unsigned short> > listeners; // listen_addresses
	std::vector<std::string> server_names;

	// File serving
	std::string root; // document_root
	std::string index; // default_index
	bool autoindex;

	// Network configuration
	int backlog; // listen_backlog;

	// Keep-Alive configuration
	int keepalive_timeout; // seconds
	int keepalive_max_requests; // max requests per connection

	// HTTP behavior
	std::vector<std::string> allow_methods;
	std::map<int, std::string> error_pages;
	int client_max_body_size;

	// CGI configuration
	std::vector<std::string> cgi_path; // cgi_interpreters
	std::vector<std::string> cgi_ext; // cgi_extensions;

	// Logging
	std::string access_log; // access_log_path
	std::string error_log; // error_log_path

	// Location blocks
	std::vector<LocationConfig> locations;
};

class Config
{
public:
	Config();

	std::vector<ConfigData> getServers() const;

	bool parseConfig(char *argv);

private:
	std::vector<ConfigData> _servers;
	bool inLocationBlock;

	bool parseConfigFile(std::ifstream &file);

	void validateConfig(ConfigData &config);

	void parseLocationConfigField(LocationConfig &config, const std::string &key,
								const std::vector<std::string> &tokens);

	template<typename ConfigT>
	void parseCommonConfigField(ConfigT &config, const std::string &key, const std::vector<std::string> &tokens);

	template<typename ConfigT>
	void parseCgiPath(ConfigT &config, const std::vector<std::string> &tokens);

	template<typename ConfigT>
	void parseCgiExt(ConfigT &config, const std::vector<std::string> &tokens);

	template<typename ConfigT>
	void parseErrorPage(ConfigT &config, const std::vector<std::string> &tokens);

	template<typename ConfigT>
	void parseAllowMethods(ConfigT &config, const std::vector<std::string> &tokens);

	template<typename ConfigT>
	void parseClientMaxBodySize(ConfigT &config, const std::vector<std::string> &tokens);

	template<typename ConfigT>
	void parseIndex(ConfigT &config, const std::vector<std::string> &tokens);

	template<typename ConfigT>
	void parseAutoindex(ConfigT &config, const std::vector<std::string> &tokens);

	template<typename ConfigT>
	void parseRoot(ConfigT &config, const std::vector<std::string> &tokens);

	void parseServerConfigField(ConfigData &config, const std::string &key, const std::vector<std::string> &tokens,
								std::ifstream &file);

	void parseLocationBlock(ConfigData &config, std::ifstream &file, const std::vector<std::string> &tokens);

	void parseListenDirective(ConfigData &config, const std::string &value);

	void parseBacklogDirective(ConfigData &config, const std::string &value);

	void parseKeepaliveTimeoutDirective(ConfigData &config, const std::string &value);

	void parseKeepaliveRequestsDirective(ConfigData &config, const std::string &value);

	void parseRedirect(LocationConfig &config, const std::vector<std::string> &tokens);

	void parseUploadStore(LocationConfig &config, const std::vector<std::string> &tokens);

	void parseUploadEnabled(LocationConfig &config, const std::vector<std::string> &tokens);

	void validateDirective(const char *const*directives, size_t count, const std::string &key);

	void strictCheckAfterServerBlock(std::ifstream &file, std::string line);
};


#endif
