#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>

struct ConfigData{

	short port;
	std::string serverName;
	std::string htmlPath;
	// Document root
	// Error pages
	// CGI settings
	// Client max body size
	// Timeouts
};

class Config {

	public:
		ConfigData GetConfigData() const;

	private:
		ConfigData _configData;
};

#endif
