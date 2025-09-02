#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <cstdint>
#include <map>

const size_t MAX_CLIENT_BODY_SIZE = 1024 * 1024 * 1024; // 1GB

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
	LocationConfig();
};

struct ConfigData{
	std::string host;
	uint16_t    port;
	std::string server_name;
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
	ConfigData();
};

class Config {

	public:
		ConfigData getConfigData() const;
		bool parseConfig(const std::string& path);

	private:
		ConfigData _configData;
};

#endif
