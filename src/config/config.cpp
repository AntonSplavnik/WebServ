#include "config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

Config::Config() : _configData() {}

LocationConfig::LocationConfig()
    : path("/"),
      autoindex(false),
      index("index.html"),
      root("./www"),
      allow_methods(),
      error_pages(),
      cgi_path(),
      cgi_ext(),
      client_max_body_size(1024 * 1024) {}

ConfigData::ConfigData()
    : host("0.0.0.0"),
      port(8080),
      server_name("localhost"),
      root("./www"),
      index("index.html"),
      backlog(128),
      access_log(""),
      error_log(""),
      autoindex(false),
      locations(),
      cgi_path(),
      cgi_ext(),
      error_pages(),
      allow_methods(),
      client_max_body_size(1024 * 1024) {}

ConfigData Config::getConfigData() const {
    return _configData;
}

// Helper to validate HTTP methods
bool isValidHttpMethod(const std::string& method) {
    static const char* valid[] = {
        "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH", "TRACE", "CONNECT"
    };
    return std::find(valid, valid + 9, method) != (valid + 9);
}

// Helper to validate autoindex values
bool isValidAutoindexValue(const std::string& value) {
    return value == "on" || value == "off" ||
           value == "true" || value == "false" ||
           value == "1" || value == "0";
}
// Helper to add unique values from 'src' to 'dest'
template<typename T>
void addUnique(std::vector<T>& dest, const std::vector<T>& src) {
    for (size_t i = 0; i < src.size(); ++i) {
        if (std::find(dest.begin(), dest.end(), src[i]) == dest.end()) {
            dest.push_back(src[i]);
        }
    }
}

// Helper to parse common config fields
template<typename ConfigT>
void parseCommonConfigField(ConfigT& config, const std::string& key, const std::vector<std::string>& tokens) {
    if (key == "autoindex" && !tokens.empty()) {
        if (!isValidAutoindexValue(tokens[0])) {
            std::cerr << "Warning: Invalid autoindex value '" << tokens[0] << "'. Set to default: " << config.autoindex << std::endl;
        return; // Skip this directive, continue parsing
        }
        config.autoindex = (tokens[0] == "on" || tokens[0] == "true" || tokens[0] == "1");
    } else if (key == "index" && !tokens.empty()) {
        config.index = tokens[0];
    } else if (key == "root" && !tokens.empty()) {
        config.root = tokens[0];
    } else if (key == "allow_methods" && !tokens.empty()) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (!isValidHttpMethod(tokens[i])) {
            std::cerr << "Warning: Invalid HTTP method '" << tokens[i] << "' in allow_methods. Skipping.\n";
            continue;
        }
        if (std::find(config.allow_methods.begin(), config.allow_methods.end(), tokens[i]) == config.allow_methods.end()) {
            config.allow_methods.push_back(tokens[i]);
        }
    }
    } else if (key == "error_page" && tokens.size() >= 2) {
        std::string path = tokens.back();
        for (size_t i = 0; i < tokens.size() - 1; ++i) {
            int code = 0;
            std::istringstream codeStream(tokens[i]);
            if (codeStream >> code) {
                config.error_pages[code] = path;
            }
        }
        }
    else if (key == "cgi_path" && !tokens.empty()) {
        addUnique(config.cgi_path, tokens);
    } else if (key == "cgi_ext" && !tokens.empty()) {
        addUnique(config.cgi_ext, tokens);
    }
    else if (key == "client_max_body_size" && !tokens.empty()) {
        std::istringstream iss(tokens[0]);
        int size = 0;
        if (!(iss >> size)) {
            std::cerr << "Warning: Invalid client_max_body_size value '" << tokens[0] << "'. Set to default or previous line: " << config.client_max_body_size << std::endl;
            return;
        }
        if (size > MAX_CLIENT_BODY_SIZE) {
            std::cerr << "Warning: client_max_body_size (" << size << ") exceeds maximum allowed (" << MAX_CLIENT_BODY_SIZE << "). Set to INT maximum: " << config.client_max_body_size << std::endl;
            return;
        }
        config.client_max_body_size = size;
    }

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
        if (key == "location" && !tokens.empty()) {
    LocationConfig loc;
    loc.path = tokens[0];

    // Check for '{' at end of line or next line
    bool foundBrace = false;
    std::cout << "Reading location for path: " << loc.path << std::endl;
    std::cout << "tokens" << std::endl;
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << "  Token[" << i << "]: '" << tokens[i]
                    << "'" << std::endl;
    }
    std::cout << "Line: '" << line << "'" << std::endl;
    if (!tokens.empty() && tokens.back() == "{") {
    foundBrace = true;
} else if (!loc.path.empty() && loc.path.back() == '{') {
    foundBrace = true;
    loc.path.pop_back();
}

    std::cout << "Found brace on same line: " << (foundBrace ? "yes" : "no") << std::endl;
    if (!foundBrace) {
        // Try next line for '{'
        if (!std::getline(file, line)) continue;
        std::istringstream brace_iss(line);
        std::string maybeBrace;
        if (!(brace_iss >> maybeBrace) || maybeBrace != "{") continue;
    }
	std::cout << "Parsing location block for path: " << loc.path << std::endl;
    // Parse the location block
    while (std::getline(file, line)) {
        // Remove possible trailing '}'
        size_t closePos = line.find('}');
        bool blockEnd = (closePos != std::string::npos);
        std::string lineContent = blockEnd ? line.substr(0, closePos) : line;

        std::istringstream liss(lineContent);
        std::string lkey;
        if (!(liss >> lkey)) {
            if (blockEnd) break;
            continue;
        }
        std::vector<std::string> ltokens = readValues(liss);
        parseCommonConfigField(loc, lkey, ltokens);
        if (blockEnd) break;
    }
    _configData.locations.push_back(loc);
    continue;
}
        else if (key == "listen" && !tokens.empty()) {
    std::string value = tokens[0];
    size_t colon = value.find(':');
    int port = 0;
    if (colon != std::string::npos) {
        _configData.host = value.substr(0, colon);
        std::istringstream portStream(value.substr(colon + 1));
        if (portStream >> port && port >= 1 && port <= 65535) {
            _configData.port = static_cast<uint16_t>(port);
        } else {
            std::cerr << "Warning: Invalid port value '" << value.substr(colon + 1) << "'. Set to previous or default: " << _configData.port << std::endl;
        }
    } else {
        std::istringstream portStream(value);
        if (portStream >> port && port >= 1 && port <= 65535) {
            _configData.port = static_cast<uint16_t>(port);
        } else {
            std::cerr << "Warning: Invalid port value '" << value.substr(colon + 1) << "Set to previous or default: " << _configData.port << std::endl;
        }
            }
        }
        else if (key == "host" && !tokens.empty()) {
            _configData.host = tokens[0];
        }
        else if (key == "port" && !tokens.empty()) {
    int port = 0;
    std::istringstream valStream(tokens[0]);
    std::cout << "Parsing port value: '" << tokens[0] << "'" << std::endl;
    if (!(valStream >> port) || port < 1 || port > 65535) {
        std::cerr << "Warning: Invalid port value '" << tokens[0] << "'. Skipping.\n";
        continue;
    }
    _configData.port = static_cast<uint16_t>(port);
}
        else if (key == "server_name" && !tokens.empty()) {
            _configData.server_name = tokens[0];
        }
        else if (key == "backlog" && !tokens.empty()) {
    int backlog = 0;
    std::istringstream valStream(tokens[0]);
    if (!(valStream >> backlog) || backlog < 1 || backlog > MAX_BACKLOG) {
        std::cerr << "Warning: Invalid backlog value '" << tokens[0]
                  << "'. Must be between 1 and " << MAX_BACKLOG
                  << ". Set to default or previous line: " << _configData.backlog << std::endl;
        continue;
    }
    _configData.backlog = backlog;
        }
        else if (key == "access_log" && !tokens.empty()) {
            _configData.access_log = tokens[0];
        }
        else if (key == "error_log" && !tokens.empty()) {
            _configData.error_log = tokens[0];
        }
        else {
            parseCommonConfigField(_configData, key, tokens);
        }
    }
    return true;
}