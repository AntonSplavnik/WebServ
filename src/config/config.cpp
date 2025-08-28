#include "config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

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

// Helper to read all values from iss, stripping trailing semicolons
std::vector<std::string> readValues(std::istringstream& iss) {
    std::vector<std::string> values;
    std::string value;
    while (iss >> value) {
        if (!value.empty() && value.back() == ';') value.pop_back();
        values.push_back(value);
    }
    return values;
}
// Loads configuration data from a file at the given path.
// Returns true if the file was successfully read and parsed, false otherwise
bool Config::parseConfig(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key)) continue;
        std::vector<std::string> tokens = readValues(iss);

        if (key == "listen" && !tokens.empty()) {
            std::string value = tokens[0];
            size_t colon = value.find(':');
            if (colon != std::string::npos) {
                _configData.host = value.substr(0, colon);
                int port = 0;
                std::istringstream portStream(value.substr(colon + 1));
                if (portStream >> port) _configData.port = static_cast<uint16_t>(port);
            }
        }
        else if (key == "host" && !tokens.empty()) {
            _configData.host = tokens[0];
        }
        else if (key == "port" && !tokens.empty()) {
            int port = 0;
            std::istringstream valStream(tokens[0]);
            if (valStream >> port) _configData.port = static_cast<uint16_t>(port);
        }
        else if (key == "server_name" && !tokens.empty()) {
            _configData.server_name = tokens[0];
        }
        else if (key == "root" && !tokens.empty()) {
            _configData.root = tokens[0];
        }
        else if (key == "index" && !tokens.empty()) {
            _configData.index = tokens[0];
        }
        else if (key == "backlog" && !tokens.empty()) {
            int backlog = 0;
            std::istringstream valStream(tokens[0]);
            if (valStream >> backlog) _configData.backlog = backlog;
        }
        else if (key == "error_page" && tokens.size() >= 2) {
            std::string path = tokens.back();
            for (size_t i = 0; i < tokens.size() - 1; ++i) {
                int code = 0;
                std::istringstream codeStream(tokens[i]);
                if (codeStream >> code) {
                    _configData.error_pages[code] = path;
                }
            }
        }
        else if (key == "access_log" && !tokens.empty()) {
            _configData.access_log = tokens[0];
        }
        else if (key == "error_log" && !tokens.empty()) {
            _configData.error_log = tokens[0];
        }
        else if (key == "cgi_path") {
            for (size_t i = 0; i < tokens.size(); ++i) {
                if (std::find(_configData.cgi_path.begin(), _configData.cgi_path.end(), tokens[i]) == _configData.cgi_path.end()) {
                    _configData.cgi_path.push_back(tokens[i]);
                }
            }
        }
        else if (key == "cgi_ext") {
            for (size_t i = 0; i < tokens.size(); ++i) {
                if (std::find(_configData.cgi_ext.begin(), _configData.cgi_ext.end(), tokens[i]) == _configData.cgi_ext.end()) {
                    _configData.cgi_ext.push_back(tokens[i]);
                }
            }
        }
        else if (key == "autoindex" && !tokens.empty()) {
            std::string value = tokens[0];
            _configData.autoindex = (value == "on" || value == "true" || value == "1");
        }
    }
    return true;
}