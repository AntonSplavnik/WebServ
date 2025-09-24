#include "config.hpp"
#include "../helpers/helpers.hpp"
#include "../exceptions/config_exceptions.hpp"
#include "../logging/logger.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sstream>

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
      server_names(),
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

void Config::validateConfig(ConfigData& config) {
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
    if (config.listeners.empty())
    	throw ConfigParseException("Missing required config: at least one listen directive");
    if (config.error_pages.empty()) {
        config.error_pages[404] = DEFAULT_ERROR_PAGE_404;
        config.error_pages[500] = DEFAULT_ERROR_PAGE_500;
        config.error_pages[403] = DEFAULT_ERROR_PAGE_403;
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
        if (loc.error_pages.empty())
    		loc.error_pages = config.error_pages;

        // Upload validation
        if (loc.upload_enabled) {
    if (loc.upload_store.empty())
        throw ConfigParseException("upload_enabled is on but upload_store is not set in location: " + loc.path);
    if (!isValidPath(loc.upload_store, W_OK | X_OK))
        throw ConfigParseException("Inaccessible upload_store path for location " + loc.path + ": " + loc.upload_store);
}

        // Redirect validation
        if (!loc.redirect.empty() && (loc.redirect_code < 300 || loc.redirect_code > 399))
            throw ConfigParseException("Invalid redirect code in location " + loc.path + ": " + std::to_string(loc.redirect_code));
        if (!loc.autoindex) loc.autoindex = config.autoindex;

    }
}

// Parsing of the location-specific config fields
void parseLocationConfigField(LocationConfig& config, const std::string& key, const std::vector<std::string>& tokens) {
    if (key == "upload_enabled" && !tokens.empty()) {
        const std::string& val = tokens[0];
        if (val == "on" || val == "true" || val == "1") {
            config.upload_enabled = true;
        } else if (val == "off" || val == "false" || val == "0") {
            config.upload_enabled = false;
        } else {
            throw ConfigParseException("Invalid upload_enabled value: " + val);
        }
    }
    else if (key == "upload_store" && !tokens.empty()) {
        if (!isValidPath(tokens[0], W_OK | X_OK))
            throw ConfigParseException("Invalid or inaccessible upload_store path: " + tokens[0]);
        config.upload_store = normalizePath(tokens[0]);
    }
    else if (key == "redirect" && !tokens.empty()) {
        if (tokens.size() < 2)
            throw ConfigParseException("Redirect directive requires status code and target path/URL");
        int code = std::atoi(tokens[0].c_str());
        if (isValidHttpStatusCode(code) && (code == 301 || code == 302 || code == 303)) {
            config.redirect_code = code;
            config.redirect = tokens[1];
        } else {
            throw ConfigParseException("Invalid redirect status code: " + tokens[0]);
        }
        if (tokens.size() > 2)
            throw ConfigParseException("Too many arguments for redirect directive");
        std::cout << "Redirect set to: " << config.redirect << " with code: " << config.redirect_code << std::endl;
    }
}

// Helper to parse common config fields
template<typename ConfigT>
void parseCommonConfigField(ConfigT& config, const std::string& key, const std::vector<std::string>& tokens) {
    if (key == "autoindex" && !tokens.empty()) {
        if (!isValidAutoindexValue(tokens[0]))
          throw ConfigParseException("Invalid autoindex value: " + tokens[0]);
        config.autoindex = (tokens[0] == "on" || tokens[0] == "true" || tokens[0] == "1");
    } else if (key == "root" && !tokens.empty()) {
    if (!config.root.empty())
        throw ConfigParseException("Duplicate root directive");
    if (!isValidPath(tokens[0], R_OK | X_OK))
        throw ConfigParseException("Invalid or inaccessible root path: " + tokens[0]);
    config.root = normalizePath(tokens[0]);
} else if (key == "index" && !tokens.empty()) {
    if (!config.index.empty())
        throw ConfigParseException("Duplicate index directive");
    if (!isValidFile(tokens[0], R_OK))
        throw ConfigParseException("Invalid or inaccessible index file: " + tokens[0]);
    config.index = tokens[0];
} else if (key == "client_max_body_size" && !tokens.empty()) {
    if (config.client_max_body_size > 0)
        throw ConfigParseException("Duplicate client_max_body_size directive");
    std::istringstream iss(tokens[0]);
    int size = 0;
    if (!(iss >> size))
        throw ConfigParseException("Invalid client_max_body_size value: " + tokens[0]);
    if (size > MAX_CLIENT_BODY_SIZE)
        throw ConfigParseException("client_max_body_size (" + tokens[0] + ") exceeds maximum allowed (" + std::to_string(MAX_CLIENT_BODY_SIZE) + ")");
    config.client_max_body_size = size;
}  else if (key == "allow_methods" && !tokens.empty()) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (!isValidHttpMethod(tokens[i]))
            throw ConfigParseException("Invalid HTTP method in allow_methods: " + tokens[i]);
        addUnique(config.allow_methods, tokens[i]);
    }
}
     else if (key == "error_page" && tokens.size() >= 2) {
    std::string path = tokens.back();
    if (!isValidFile(path, R_OK))
      throw ConfigParseException("Invalid or inaccessible error_page file: " + path);
    for (size_t i = 0; i < tokens.size() - 1; ++i) {
    if (tokens[i].empty()) continue; // Skip empty tokens
    int code = 0;
    std::istringstream codeStream(tokens[i]);
    if (codeStream >> code) {
        if (!isValidHttpStatusCode(code))
            throw ConfigParseException("Invalid HTTP status code in error_page: " + tokens[i]);
        config.error_pages[code] = path;
    }
}
}
else if (key == "cgi_ext" && !tokens.empty()) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (!isValidCgiExt(tokens[i]))
          throw ConfigParseException("Invalid CGI extension: " + tokens[i]);
        addUnique(config.cgi_ext, tokens[i]);
        }
    }

    else if (key == "cgi_path" && !tokens.empty()) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (!isValidPath(tokens[i], X_OK))
          throw ConfigParseException("Invalid CGI path: " + tokens[i]);
        addUnique(config.cgi_path, tokens[i]);
        }
    }


}

// Loads configuration data from a file at the given path.
// Returns true if the file was successfully read and parsed, false otherwise
bool Config::parseConfigFile(std::ifstream& file)
{
	bool inLocationBlock = false;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key)) continue;
        std::vector<std::string> tokens = readValues(iss);
        if (key == "location" && !tokens.empty()) {
         inLocationBlock = true;
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
            inLocationBlock = false;
            continue;
        }
        else {
    // List of known directives
   // Inside the location block parsing loop in parseConfig

if (inLocationBlock) {
	if (std::find(LOCATION_DIRECTIVES, LOCATION_DIRECTIVES  +  LOCATION_DIRECTIVES_COUNT , lkey) == LOCATION_DIRECTIVES +  LOCATION_DIRECTIVES_COUNT )
    	throw ConfigParseException("Unknown directive: " + lkey);
	}
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
            addUnique(_configData.server_names, tokens[0]);
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
   	if (!inLocationBlock) {
    if (std::find(SERVER_DIRECTIVES, SERVER_DIRECTIVES + SERVER_DIRECTIVES_COUNT, key) == SERVER_DIRECTIVES + SERVER_DIRECTIVES_COUNT) {
        throw ConfigParseException("Unknown directive: " + key);
    }
    }
    parseCommonConfigField(_configData, key, tokens);
}
    }
    validateConfig(_configData);
    return true;
}

bool Config::parseConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw ConfigParseException("Failed to open config file: " + path);

    parseConfigFile(file);

    return true;
}