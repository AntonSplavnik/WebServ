#include "config.hpp"
#include "../exceptions/config_exceptions.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <sys/stat.h>
#include <unistd.h>

Config::Config() : _configData() {}

LocationConfig::LocationConfig()
    : path(),
      autoindex(),
      index(),
      root(),
      allow_methods(),
      error_pages(),
      cgi_path(),
      cgi_ext(),
      client_max_body_size(),
      upload_enabled(),
      upload_store(),
      redirect(),
      redirect_code() {}

ConfigData::ConfigData()
    :
      server_name(),
      root(),
      index(),
      backlog(),
      access_log(),
      error_log(),
      autoindex(),
      locations(),
      cgi_path(),
      cgi_ext(),
      error_pages(),
      allow_methods(),
      client_max_body_size() {}

ConfigData Config::getConfigData() const {
    return _configData;
}


// Helper to normalize paths (resolve ., .., symlinks)
std::string normalizePath(const std::string& path) {
    char resolved[PATH_MAX];
    if (realpath(path.c_str(), resolved)) {
        return std::string(resolved);
    }
    return path; // fallback if path does not exist
}

// Helper to validate if a path is a regular file and has the specified access mode
bool isValidFile(const std::string& path, int mode) {
    struct stat sb;
    if (stat(path.c_str(), &sb) != 0) return false;
    return S_ISREG(sb.st_mode) && (access(path.c_str(), mode) == 0);
}

// Helper to validate if a path is a directory and has the specified access mode
bool isValidPath(const std::string& path, int mode) {
    struct stat sb;
    if (stat(path.c_str(), &sb) != 0) return false;
    if (!S_ISDIR(sb.st_mode)) return false;
    return (access(path.c_str(), mode) == 0);
}
// Helper to validate HTTP methods
bool isValidHttpMethod(const std::string& method) {
    static const char* valid[] = {
        "GET", "POST", "DELETE"
    };
    return std::find(valid, valid + 3, method) != (valid + 3);
}

// Helper to validate HTTP status codes
bool isValidHttpStatusCode(int code) {
    return code >= 100 && code <= 599;
}
// Helper to validate IPv4 addresses
bool isValidIPv4(const std::string& ip) {
    size_t start = 0, end = 0, count = 0;
    while (end != std::string::npos) {
        end = ip.find('.', start);
        std::string part = ip.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
        if (part.empty() || part.size() > 3) return false;
        for (size_t i = 0; i < part.size(); ++i)
            if (!isdigit(part[i])) return false;
        int num = std::atoi(part.c_str());
        if (num < 0 || num > 255) return false;
        start = end + 1;
        ++count;
    }
    return count == 4;
}
// Helper to validate CGI extensions
bool isValidCgiExt(const std::string& ext) {
    static const char* valid[] = { ".cgi", ".pl", ".py", ".php", ".sh", ".rb", ".js", ".asp", ".exe", ".bat", ".tcl", ".lua" };
return std::find(valid, valid + 12, ext) != (valid + 12);
}

// Helper to validate host (simple check for IP or hostname)
bool isValidHost(const std::string& host) {
    if (isValidIPv4(host)) return true;
    static const std::regex hostname("^([a-zA-Z0-9\\-\\.]+)$");
    if (std::regex_match(host, hostname)) {
        // Reject all-numeric hostnames
        if (std::find_if(host.begin(), host.end(), ::isalpha) == host.end())
            return false;
        return true;
    }
    return false;
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

// Helper to assign log file paths with validation and creation if needed
bool assignLogFile(std::string& logField, const std::string& path) {
    if (!isValidFile(path, W_OK) && !path.empty()) {
        std::ofstream ofs(path.c_str(), std::ios::app);
        if (!ofs)
        	throw ConfigParseException("Cannot create or open log file: " + path);
		else
            std::cout << "Info: Created log file '" << path << "'." << std::endl;
    }
    logField = path;
    return true;
}

// Helper to parse location-specific config fields
void parseLocationConfigField(LocationConfig& config, const std::string& key, const std::vector<std::string>& tokens) {
    if (key == "upload_enabled" && !tokens.empty()) {
        config.upload_enabled = (tokens[0] == "on" || tokens[0] == "true" || tokens[0] == "1");
    }
    else if (key == "upload_store" && !tokens.empty()) {
        if (!isValidPath(tokens[0], W_OK | X_OK))
      		throw ConfigParseException("Invalid or inaccessible upload_store path: " + tokens[0]);
        config.upload_store = normalizePath(tokens[0]);
    }
    else if (key == "redirect" && !tokens.empty()) {
        if (tokens.size() >= 2) {
            int code = std::atoi(tokens[0].c_str());
            if (isValidHttpStatusCode(code) && (code == 301 || code == 302 || code == 303)) {
                config.redirect_code = code;
                config.redirect = tokens[1];
            } else
                throw ConfigParseException("Invalid redirect status code: " + tokens[0]);
            if (tokens.size() > 2)
              throw ConfigParseException("Too many arguments for redirect directive");
        } else {
            config.redirect = tokens[0];
            config.redirect_code = 302; // Default to 302 if no status code is provided
        }
        std::cout << "Redirect set to: " << config.redirect << " with code: " << config.redirect_code << std::endl;
    }
}

void validateConfig(ConfigData& config) {
    // --- Top-level required fields ---
    if (config.root.empty())
        throw ConfigParseException("Missing required config: root");
    if (config.index.empty() && !config.autoindex)
        throw ConfigParseException("Missing required config: index (and autoindex is off)");
    if (config.backlog <= 0)
        throw ConfigParseException("Missing or invalid required config: backlog");
    if (config.access_log.empty())
        throw ConfigParseException("Missing required config: access_log");
    if (config.error_log.empty())
        throw ConfigParseException("Missing required config: error_log");
    if (config.client_max_body_size <= 0)
        throw ConfigParseException("Missing or invalid required config: client_max_body_size");

    if (config.allow_methods.empty()) {
        config.allow_methods.push_back("GET");
        std::cout << "Info: No allow_methods specified, defaulting to GET" << std::endl;
    }

    // --- Each location ---
    for (size_t i = 0; i < config.locations.size(); ++i) {
        LocationConfig& loc = config.locations[i];

        // Fallbacks
        if (loc.root.empty()) loc.root = config.root;
        if (loc.index.empty()) loc.index = config.index;
        if (loc.allow_methods.empty()) {
            loc.allow_methods = config.allow_methods;
            if (loc.allow_methods.empty()) {
                loc.allow_methods.push_back("GET");
                std::cout << "Info: No allow_methods specified in location, defaulting to GET" << std::endl;
            }
        }
        if (loc.client_max_body_size <= 0)
            loc.client_max_body_size = config.client_max_body_size;

        // Validation after fallback
        // Check location path (URI)
		if (loc.path.empty() || loc.path[0] != '/')
    		throw ConfigParseException("Invalid location config: path must start with '/': " + loc.path);

		// Check root (filesystem directory)
		if (loc.root.empty())
    		throw ConfigParseException("Missing required location config: root");
		if (!isValidPath(loc.root, R_OK | X_OK))
    		throw ConfigParseException("Inaccessible root path for location " + loc.path + ": " + loc.root);

        if (loc.index.empty() && loc.autoindex == false)
            throw ConfigParseException("Missing index and autoindex is off in location: " + loc.path);
        if (loc.allow_methods.empty())
            throw ConfigParseException("Missing required location config: allow_methods");
        if (loc.client_max_body_size <= 0)
            throw ConfigParseException("Missing required location config: client_max_body_size");
        // CGI validation
        if (loc.cgi_ext.empty()) loc.cgi_ext = config.cgi_ext;
        if (loc.cgi_path.empty()) loc.cgi_path = config.cgi_path;
        if (!loc.cgi_ext.empty() && loc.cgi_path.empty())
            throw ConfigParseException("Missing required location config: cgi_path for CGI");
        if (!loc.cgi_path.empty() && loc.cgi_ext.empty())
            throw ConfigParseException("Missing required location config: cgi_ext for CGI");
    }
}



// Helper to parse common config fields
template<typename ConfigT>
void parseCommonConfigField(ConfigT& config, const std::string& key, const std::vector<std::string>& tokens) {
    if (key == "autoindex" && !tokens.empty()) {
        if (!isValidAutoindexValue(tokens[0]))
          throw ConfigParseException("Invalid autoindex value: " + tokens[0]);
        config.autoindex = (tokens[0] == "on" || tokens[0] == "true" || tokens[0] == "1");
    } else if (key == "index" && !tokens.empty()) {
    if (!isValidFile(tokens[0], R_OK))
      throw ConfigParseException("Invalid or inaccessible index file: " + tokens[0]);
    config.index = tokens[0];
    } else if (key == "root" && !tokens.empty()) {
    if (!isValidPath(tokens[0], R_OK | X_OK))
      throw ConfigParseException("Invalid or inaccessible root path: " + tokens[0]);
    config.root = normalizePath(tokens[0]);
    } else if (key == "allow_methods" && !tokens.empty()) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (!isValidHttpMethod(tokens[i]))
          throw ConfigParseException("Invalid HTTP method in allow_methods: " + tokens[i]);
        if (std::find(config.allow_methods.begin(), config.allow_methods.end(), tokens[i]) == config.allow_methods.end()) {
            config.allow_methods.push_back(tokens[i]);
        }
    }
    } else if (key == "error_page" && tokens.size() >= 2) {
    std::string path = tokens.back();
    if (!isValidFile(path, R_OK))
      throw ConfigParseException("Invalid or inaccessible error_page file: " + path);
    for (size_t i = 0; i < tokens.size() - 1; ++i) {
        int code = 0;
        std::istringstream codeStream(tokens[i]);
        if (codeStream >> code) {
            if (!isValidHttpStatusCode(code))
              throw ConfigParseException("Invalid HTTP status code in error_page: " + tokens[i]);
        }
    }
}
else if (key == "cgi_ext" && !tokens.empty()) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (!isValidCgiExt(tokens[i]))
          throw ConfigParseException("Invalid CGI extension: " + tokens[i]);
        if (std::find(config.cgi_ext.begin(), config.cgi_ext.end(), tokens[i]) == config.cgi_ext.end()) {
            config.cgi_ext.push_back(tokens[i]);
        }
    }
}
    else if (key == "cgi_path" && !tokens.empty()) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (!isValidPath(tokens[i], X_OK))
          throw ConfigParseException("Invalid CGI path: " + tokens[i]);
        if (std::find(config.cgi_path.begin(), config.cgi_path.end(), tokens[i]) == config.cgi_path.end()) {
            config.cgi_path.push_back(tokens[i]);
        }
    }
}
    else if (key == "client_max_body_size" && !tokens.empty()) {
        std::istringstream iss(tokens[0]);
        int size = 0;
        if (!(iss >> size))
          throw ConfigParseException("Invalid client_max_body_size value: " + tokens[0]);
        if (size > MAX_CLIENT_BODY_SIZE)
          throw ConfigParseException("client_max_body_size (" + tokens[0] + ") exceeds maximum allowed (" + std::to_string(MAX_CLIENT_BODY_SIZE) + ")");
        config.client_max_body_size = size;
    }
}

// Helper to read all values from iss, stripping trailing semicolons
std::vector<std::string> readValues(std::istringstream& iss) {
    std::vector<std::string> values;
    std::string value;
    while (iss >> value) {
    if (!value.empty() && value.back() == ';') value.pop_back();
    if (!value.empty()) values.push_back(value);
}
    return values;
}
// Loads configuration data from a file at the given path.
// Returns true if the file was successfully read and parsed, false otherwise
bool Config::parseConfig(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
      throw ConfigParseException("Failed to open config file: " + path);

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
        parseLocationConfigField(loc, lkey, ltokens);
        if (blockEnd) break;
    }
    _configData.locations.push_back(loc);
    continue;
}
else if (key == "listen" && !tokens.empty()) {
    std::string value = tokens[0];
    size_t colon = value.find(':');
    std::string host = "0.0.0.0";
    int port = 80; // default port
    if (colon != std::string::npos) {
        host = value.substr(0, colon);
        std::string portPart = value.substr(colon + 1);
        if (isValidIPv4(host)) {
            // Valid IPv4
        } else if (isValidHost(host)) {
            // Valid hostname
        } else {
            throw ConfigParseException("Invalid host in listen directive: " + host);
        }
        std::istringstream portStream(portPart);
        if (!(portStream >> port) || port < 1 || port > 65535)
            throw ConfigParseException("Invalid port in listen directive: " + portPart);
    } else if (isValidIPv4(value)) {
        host = value;
        port = 80;
    } else if (isValidHost(value)) {
        host = value;
        port = 80;
    } else {
        throw ConfigParseException("Invalid listen directive: " + value);
    }
    std::pair<std::string,uint16_t> listenPair(host, static_cast<uint16_t>(port));
    if (std::find(_configData.listeners.begin(),
                  _configData.listeners.end(),
                  listenPair) != _configData.listeners.end()) {
        throw ConfigParseException("Duplicate listen directive: " + host + ":" + std::to_string(port));
    }
    _configData.listeners.push_back(std::make_pair(host, static_cast<uint16_t>(port)));
}
        else if (key == "server_name" && !tokens.empty()) {
            _configData.server_name = tokens[0];
        }
        else if (key == "backlog" && !tokens.empty()) {
    int backlog = 0;
    std::istringstream valStream(tokens[0]);
    if (!(valStream >> backlog) || backlog < 1 || backlog > MAX_BACKLOG)
    	throw ConfigParseException("Invalid backlog value: " + tokens[0]);
    _configData.backlog = backlog;
        }
        else if (key == "access_log" && !tokens.empty()) {
    assignLogFile(_configData.access_log, tokens[0]);
}
else if (key == "error_log" && !tokens.empty()) {
    assignLogFile(_configData.error_log, tokens[0]);
}
        else {
    // List of known directives
    static const char* knownDirectivesArr[] = {
        "server", "location", "listen", "server_name", "backlog",
        "access_log", "error_log", "autoindex", "index", "root",
        "allow_methods", "error_page", "cgi_ext", "cgi_path", "client_max_body_size",
        "upload_enabled", "upload_store", "redirect"
    };
    static const size_t knownDirectivesCount = sizeof(knownDirectivesArr) / sizeof(knownDirectivesArr[0]);
    if (std::find(knownDirectivesArr, knownDirectivesArr + knownDirectivesCount, key) == knownDirectivesArr + knownDirectivesCount) {
        throw ConfigParseException("Unknown directive: " + key);
    }
    parseCommonConfigField(_configData, key, tokens);
}
    }
    validateConfig(_configData);
    return true;
}