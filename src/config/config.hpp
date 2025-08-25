#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <cstdint>

struct ConfigData{
	std::string host;
	uint16_t    port;
	std::string server_name;
	std::string root;
	std::string index;
	int         backlog;

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
