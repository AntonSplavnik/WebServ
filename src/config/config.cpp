#include "config.hpp"
#include <fstream>
#include <sstream>

ConfigData::ConfigData()
    : host("0.0.0.0"),
      port(8080),
      server_name("localhost"),
      root("./www"),
      index("index.html"),
      backlog(128)
{}

ConfigData Config::getConfigData() const {
    return _configData;
}

// Loads configuration data from a file at the given path.
// Returns true if the file was successfully read and parsed, false otherwise
bool Config::parseConfig(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string key, value;
        if (!(iss >> key >> value)) continue;
        if (key == "listen") {
    	// Remove trailing semicolon if present
    	if (!value.empty() && value.back() == ';') value.pop_back();
    	size_t colon = value.find(':');
    	if (colon != std::string::npos) {
        	_configData.host = value.substr(0, colon);
        	std::istringstream portStream(value.substr(colon + 1));
        	int port;
        	if (portStream >> port) _configData.port = static_cast<uint16_t>(port);
    }
}
        if (key == "host") _configData.host = value;
        else if (key == "port") {
            std::istringstream valStream(value);
            int port;
            if (valStream >> port) _configData.port = static_cast<uint16_t>(port);
        }
        else if (key == "server_name") _configData.server_name = value;
        else if (key == "root") _configData.root = value;
        else if (key == "index") _configData.index = value;
        else if (key == "backlog") {
            std::istringstream valStream(value);
            int backlog;
            if (valStream >> backlog) _configData.backlog = backlog;
        }
    }
    return true;
}