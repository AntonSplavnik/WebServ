#ifndef CONFIG_HPP
#define CONFIG_HPP
#include <string>
#include <map>
#include <vector>

const int MAX_BACKLOG = 1024;
const size_t MAX_CLIENT_BODY_SIZE = 1024 * 1024 * 1024; // 1GB

// Valid CGI extensions
static const char* VALID_CGI_EXTENSIONS[] = { ".cgi", ".pl", ".py", ".php", ".sh", ".rb", ".js", ".asp", ".exe", ".bat", ".tcl", ".lua" };
static const size_t VALID_CGI_EXTENSIONS_COUNT = sizeof(VALID_CGI_EXTENSIONS) / sizeof(VALID_CGI_EXTENSIONS[0]);

// Valid autoindex values
static const char* AUTOINDEX_VALUES[] = { "on", "off", "true", "false", "1", "0" };
static const size_t AUTOINDEX_VALUES_COUNT = sizeof(AUTOINDEX_VALUES) / sizeof(AUTOINDEX_VALUES[0]);

// Valid HTTP methods
static const char* HTTP_METHODS[] = { "GET", "POST", "DELETE"};
static const size_t HTTP_METHODS_COUNT = sizeof(HTTP_METHODS) / sizeof(HTTP_METHODS[0]);

//Valid location directives (used in config.cpp)
static const char* LOCATION_DIRECTIVES[] = {
	"autoindex", "root", "index", "allow_methods", "cgi_ext", "cgi_path",
	"upload_enabled", "upload_store", "redirect", "error_page", "client_max_body_size"
};
static const size_t LOCATION_DIRECTIVES_COUNT = sizeof(LOCATION_DIRECTIVES) / sizeof(LOCATION_DIRECTIVES[0]);

//Valid server directives (used in config.cpp)
static const char* SERVER_DIRECTIVES[] = {
	"location", "listen", "server_name", "backlog",
		"access_log", "error_log", "autoindex", "index", "root",
		"allow_methods", "error_page", "cgi_ext", "cgi_path", "client_max_body_size",
		 "redirect"
};
static const size_t SERVER_DIRECTIVES_COUNT = sizeof(SERVER_DIRECTIVES) / sizeof(SERVER_DIRECTIVES[0]);

// Default error pages
static const char* DEFAULT_ERROR_PAGE_404 = "runtime/www/errors/40x.html";
static const char* DEFAULT_ERROR_PAGE_500 = "runtime/www/errors/50x.html";
static const char* DEFAULT_ERROR_PAGE_403 = "runtime/www/errors/40x.html";
static const char* DEFAULT_ERROR_PAGE_413 = "runtime/www/errors/40x.html";



struct LocationConfig {
	std::string path;
	bool autoindex;
	std::string index;
	std::string root;
	std::vector<std::string> allow_methods;
	std::map <int, std::string> error_pages;
	std::vector<std::string> cgi_path;
	std::vector<std::string> cgi_ext;
	int client_max_body_size;
	bool upload_enabled;
	std::string upload_store;
	std::string redirect;
	int redirect_code;
	LocationConfig();
};

struct ConfigData{
	std::vector<std::pair<std::string, unsigned short> > listeners;
	std::vector<std::string> server_names;
	std::string root;
	std::string index;
	int         backlog;
	std::vector<std::string> allow_methods;
	std::map <int, std::string> error_pages;
	std::string access_log;
	std::string error_log;
	std::vector<std::string> cgi_path;
	std::vector<std::string> cgi_ext;
	bool	autoindex;
	std::vector<LocationConfig> locations;
	int client_max_body_size;
	std::string upload_store;
	ConfigData();
};

class Config {

	public:
	    Config();
		std::vector<ConfigData> getServers() const;
		bool					parseConfig(char **argv);

	private:
		std::vector<ConfigData> _servers;
		bool					inLocationBlock;
		bool					parseConfigFile(std::ifstream& file);
		void					validateConfig(ConfigData& config);
		void					parseLocationConfigField(LocationConfig& config, const std::string& key, const std::vector<std::string>& tokens);
		template<typename ConfigT>
		void					parseCommonConfigField(ConfigT& config, const std::string& key, const std::vector<std::string>& tokens);
		template<typename ConfigT>
		void					parseCgiPath(ConfigT& config, const std::vector<std::string>& tokens);
		template<typename ConfigT>
		void					parseCgiExt(ConfigT& config, const std::vector<std::string>& tokens);
		template<typename ConfigT>
		void					parseErrorPage(ConfigT& config, const std::vector<std::string>& tokens);
		template<typename ConfigT>
		void					parseAllowMethods(ConfigT& config, const std::vector<std::string>& tokens);
		template<typename ConfigT>
		void					parseClientMaxBodySize(ConfigT& config, const std::vector<std::string>& tokens);
		template<typename ConfigT>
		void					parseIndex(ConfigT& config, const std::vector<std::string>& tokens);
		template<typename ConfigT>
		void					parseAutoindex(ConfigT& config, const std::vector<std::string>& tokens);
		template<typename ConfigT>
		void					parseRoot(ConfigT& config, const std::vector<std::string>& tokens);
		void					parseServerConfigField(ConfigData& config, const std::string& key, const std::vector<std::string>& tokens, std::ifstream& file);
		void					parseLocationBlock(ConfigData& config, std::ifstream& file, const std::vector<std::string>& tokens);
		void					parseListenDirective(ConfigData& config, const std::string& value);
		void					parseBacklogDirective(ConfigData& config, const std::string& value);
		void					parseRedirect(LocationConfig& config, const std::vector<std::string>& tokens);
		void					parseUploadStore(LocationConfig& config, const std::vector<std::string>& tokens);
		void					parseUploadEnabled(LocationConfig& config, const std::vector<std::string>& tokens);
		void					validateDirective(const char* const* directives, size_t count, const std::string& key);
		void					strictCheckAfterServerBlock(std::ifstream& file, std::string line);
};



#endif
