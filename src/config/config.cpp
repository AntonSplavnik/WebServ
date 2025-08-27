#include "config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

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
    std::string key;
    if (!(iss >> key)) continue;

    if (key == "listen") {
        std::string value;
        if (!(iss >> value)) continue;
        if (!value.empty() && value.back() == ';') value.pop_back();
        size_t colon = value.find(':');
        if (colon != std::string::npos) {
            _configData.host = value.substr(0, colon);
            std::istringstream portStream(value.substr(colon + 1));
            int port;
            if (portStream >> port) _configData.port = static_cast<uint16_t>(port);
        }
    }
    else if (key == "host") { std::string value; if (iss >> value) _configData.host = value; }
    else if (key == "port") { std::string value; if (iss >> value) { int port; std::istringstream valStream(value); if (valStream >> port) _configData.port = static_cast<uint16_t>(port); } }
    else if (key == "server_name") { std::string value; if (iss >> value) _configData.server_name = value; }
    else if (key == "root") { std::string value; if (iss >> value) _configData.root = value; }
    else if (key == "index") { std::string value; if (iss >> value) _configData.index = value; }
    else if (key == "backlog") { std::string value; if (iss >> value) { int backlog; std::istringstream valStream(value); if (valStream >> backlog) _configData.backlog = backlog; } }
    else if (key == "error_page") {
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token) tokens.push_back(token);
        if (tokens.size() >= 2) {
            std::string path = tokens.back();
            if (!path.empty() && path.back() == ';') path.pop_back();
            for (size_t i = 0; i < tokens.size() - 1; ++i) {
                int code;
                std::istringstream codeStream(tokens[i]);
                if (codeStream >> code) {
                    _configData.error_pages[code] = path;
                }
            }
        }
    }
    else if (key == "access_log") {
    std::string value;
    if (iss >> value) {
        if (!value.empty() && value.back() == ';') value.pop_back();
        _configData.access_log = value;
    }
}
else if (key == "error_log") {
    std::string value;
    if (iss >> value) {
        if (!value.empty() && value.back() == ';') value.pop_back();
        _configData.error_log = value;
    }
}
}
return true;
}