#include "config.hpp"
#include "helpers.hpp"
#include "config_exceptions.hpp"
#include "logger.hpp"
#include "directives_parsers.tpp"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sstream>

//parseConfigFile reads each line, extracts the key and tokens, and calls parseServerConfigField and parseCommonConfigField.
//parseServerConfigField handles server-level directives. For a location block, it parses its contents, calling both parseCommonConfigField and parseLocationConfigField for each directive inside the block.
//This ensures that both common and location-specific config fields are parsed appropriately at their respective levels.
//This structure allows for clear separation and correct handling of server and location configuration directives


Config::Config()
{
    inLocationBlock = false;
}

LocationConfig::LocationConfig()
    : path(""),
      autoindex(false),
      index(""),
      root(""),
      allow_methods(),
      error_pages(),
      client_max_body_size(0),
      cgi_path(),
      cgi_ext(),
      upload_enabled(false),
      upload_store(""),
      redirect(""),
      redirect_code(0) {}

ConfigData::ConfigData()
    :
      listeners(),
      server_names(),
      root(""),
      index(""),
      autoindex(),
      backlog(0),
      keepalive_timeout(15),
      keepalive_max_requests(100),
      allow_methods(),
      error_pages(),
      client_max_body_size(0),
      cgi_path(),
      cgi_ext(),
      access_log(""),
      error_log(""),
      locations() {}

std::vector<ConfigData> Config::getServers() const { return _servers; }

const LocationConfig* ConfigData::findMatchingLocation(const std::string& requestPath) const {

    std::cout << "[DEBUG] RequestPath: " << requestPath << std::endl;

    const LocationConfig* bestMatch = NULL;
    size_t longestMatch = 0;

    for (size_t i = 0; i < locations.size(); i++) {

        std::cout << "[DEBUG] Checking location: " << locations[i].path << std::endl;
        size_t pathLen = locations[i].path.length();

        // Check if request path starts with this location path
        if (requestPath.compare(0, pathLen, locations[i].path) == 0) {

            // For root location "/", always matches
            if (locations[i].path == "/") {
                // Only use root if we haven't found a longer match
                if (pathLen > longestMatch) {
                    bestMatch = &locations[i];
                    longestMatch = pathLen;
                    std::cout << "[DEBUG] Root location matches (length: " << pathLen << ")" << std::endl;
                }
            }
            // For non-root locations, ensure proper path boundary
            else if (requestPath.length() == pathLen || requestPath[pathLen] == '/') {
                // This is a valid match - use it if it's longer than current best
                if (pathLen > longestMatch) {
                    bestMatch = &locations[i];
                    longestMatch = pathLen;
                    std::cout << "[DEBUG] Location matches (length: " << pathLen << ")" << std::endl;
                }
            }
        }
    }

    if (bestMatch) {
        std::cout << "[DEBUG] Best match: " << bestMatch->path << " (root: " << bestMatch->root << ")" << std::endl;
    } else {
        std::cout << "[DEBUG] No matching location found" << std::endl;
    }

    return bestMatch;
}

std::string ConfigData::getErrorPage(int statusCode, const LocationConfig* location) const {
	// Try location-specific error page first
	if (location) {
		std::map<int, std::string>::const_iterator it = location->error_pages.find(statusCode);
		if (it != location->error_pages.end()) {
			return normalizePath(root + it->second);
		}

	}

	// Fall back to server-level error page
	std::map<int, std::string>::const_iterator it = error_pages.find(statusCode);
	if (it != error_pages.end()) {
		return normalizePath(root + it->second);

	}

	return "";  // No custom error page configured
}


// Validates that the given key is in the list of known directives
void Config::validateDirective(const char* const* directives, size_t count, const std::string& key) {
    if (std::find(directives, directives + count, key) == directives + count)
        throw ConfigParseException("Unknown directive: " + key);
}

/*
validates both minimum required directives and also applies default values,
performs fallbacks, and checks for consistency and correctness of various
configuration fields (including location-specific settings, CGI, uploads,
and redirects). It ensures not only presence but also validity and proper
relationships between directives. */
void Config::validateConfig(ConfigData& config) {
    // --- Top-level required fields ---
    if (config.root.empty())
        throw ConfigParseException("Missing required config: root");
    if (config.index.empty() && !config.autoindex)
        throw ConfigParseException("Missing required config: index (and autoindex is off)");
    // Validate index file exists if specified and autoindex is off
    if (!config.index.empty() && !config.autoindex) {
        std::string fullPath = normalizePath(config.root + config.index);
        if (!isValidFile(fullPath, R_OK))
            throw ConfigParseException("Invalid or inaccessible index file: " + fullPath);
    }
    if (config.backlog <= 0)
        throw ConfigParseException("Missing or invalid required config: backlog");
    if (config.access_log.empty())
        throw ConfigParseException("Missing required config: access_log");
    if (config.error_log.empty())
        throw ConfigParseException("Missing required config: error_log");
    if (config.client_max_body_size <= 0)
        throw ConfigParseException("Missing or invalid required config: client_max_body_size");
    if (config.listeners.empty())
    	throw ConfigParseException("Missing required config: at least one listen directive");
    if (config.error_pages.empty())
    {
        config.error_pages[404] = DEFAULT_ERROR_PAGE_404;
        config.error_pages[500] = DEFAULT_ERROR_PAGE_500;
        config.error_pages[403] = DEFAULT_ERROR_PAGE_403;
        config.error_pages[413] = DEFAULT_ERROR_PAGE_413;
        std::cout << "Info: No error_pages specified, applying default error pages" << std::endl;
    }
    // Validate error page files exist (paths are relative to root)
    for (std::map<int, std::string>::const_iterator it = config.error_pages.begin();
         it != config.error_pages.end(); ++it)
    {
        std::string fullPath = normalizePath(config.root + it->second);
        if (!isValidFile(fullPath, R_OK))
        {
            std::ostringstream oss;
            oss << it->first;
            throw ConfigParseException("Invalid or inaccessible error_page file for status " + oss.str() + ": " + fullPath);
        }
    }
    if (config.allow_methods.empty())
    {
        config.allow_methods.push_back("GET");
        std::cout << "Info: No allow_methods specified, defaulting to GET" << std::endl;
    }

    // Auto-create '/' location if none exists (nginx behavior)
    bool hasRootLocation = false;
    for (size_t i = 0; i < config.locations.size(); ++i)
    {
        if (config.locations[i].path == "/")
        {
            hasRootLocation = true;
            break;
        }
    }

    if (!hasRootLocation)
    {
        LocationConfig rootLoc;
        rootLoc.path = "/";
        rootLoc.root = config.root;
        rootLoc.index = config.index;
        rootLoc.autoindex = config.autoindex;
        rootLoc.allow_methods = config.allow_methods;
        rootLoc.error_pages = config.error_pages;
        rootLoc.client_max_body_size = config.client_max_body_size;
        rootLoc.cgi_ext = config.cgi_ext;
        rootLoc.cgi_path = config.cgi_path;
        config.locations.push_back(rootLoc);
        std::cout << "Info: No '/' location found, created default from server directives" << std::endl;
    }

    // --- Each location ---
    for (size_t i = 0; i < config.locations.size(); ++i)
    {
        LocationConfig& loc = config.locations[i];
        // Fallbacks
        if (loc.root.empty()) loc.root = config.root;
        if (loc.index.empty()) loc.index = config.index;
        if (loc.allow_methods.empty())
        {
            loc.allow_methods = config.allow_methods;
            if (loc.allow_methods.empty())
            {
                loc.allow_methods.push_back("GET");
                std::cout << "Info: No allow_methods specified in location, defaulting to GET" << std::endl;
            }
        }
		// location validation
        // Inherit CGI settings from server if not set in location
        if (loc.client_max_body_size <= 0)
            loc.client_max_body_size = config.client_max_body_size;
        if (loc.error_pages.empty())
    		loc.error_pages = config.error_pages;
        if (loc.cgi_ext.empty())
          loc.cgi_ext = config.cgi_ext;
        if (loc.cgi_path.empty())
          loc.cgi_path = config.cgi_path;
        // NOTE: autoindex does NOT inherit from server (nginx behavior)
        // Each location defaults to 'off' unless explicitly set to 'on'
        // Validations
		if (loc.path.empty() || loc.path[0] != '/')
    		throw ConfigParseException("Invalid location config: path must start with '/': " + loc.path);
		if (loc.root.empty())
    		throw ConfigParseException("Missing required location config: root");
		if (!isValidPath(loc.root, R_OK | X_OK))
    		throw ConfigParseException("Inaccessible root path for location " + loc.path + ": " + loc.root);
        if (loc.index.empty() && loc.autoindex == false)
            throw ConfigParseException("Missing index and autoindex is off in location: " + loc.path);
        // Validate index file exists if specified and autoindex is off
        if (!loc.index.empty() && !loc.autoindex) {
            std::string fullPath = normalizePath(loc.root + loc.index);
            if (!isValidFile(fullPath, R_OK))
                throw ConfigParseException("Invalid or inaccessible index file in location " + loc.path + ": " + fullPath);
        }
        if (loc.allow_methods.empty())
            throw ConfigParseException("Missing required location config: allow_methods");
        if (loc.client_max_body_size <= 0)
            throw ConfigParseException("Missing required location config: client_max_body_size");
        if (!loc.cgi_ext.empty() && loc.cgi_path.empty())
            throw ConfigParseException("Missing required location config: cgi_path for CGI");
        if (!loc.cgi_path.empty() && loc.cgi_ext.empty())
            throw ConfigParseException("Missing required location config: cgi_ext for CGI");
        if (loc.upload_enabled)
        {
    		if (loc.upload_store.empty())
        		throw ConfigParseException("upload_enabled is on but upload_store is not set in location: " + loc.path);
    	if (!isValidPath(loc.upload_store, W_OK | X_OK))
       		throw ConfigParseException("Inaccessible upload_store path for location " + loc.path + ": " + loc.upload_store);
		}
        if (!loc.redirect.empty() && (loc.redirect_code < 300 || loc.redirect_code > 399))
        {
        	std::ostringstream oss;
			oss << loc.redirect_code;
            throw ConfigParseException("Invalid redirect code in location " + loc.path + ": " + oss.str());
        }
        // Validate location error page files exist (paths are relative to server root, not location root)
        for (std::map<int, std::string>::const_iterator it = loc.error_pages.begin();
             it != loc.error_pages.end(); ++it)
        {
            std::string fullPath = normalizePath(config.root + it->second);
            if (!isValidFile(fullPath, R_OK))
            {
                std::ostringstream oss;
                oss << it->first;
                throw ConfigParseException("Invalid or inaccessible error_page file for status " + oss.str() + " in location " + loc.path + ": " + fullPath);
            }
        }

    }
}

void Config::parseLocationBlock(ConfigData& config, std::ifstream& file, const std::vector<std::string>& tokens) {
  	inLocationBlock = true;
    LocationConfig loc;
    loc.path = tokens[0];

    // Check for '{' at end of line or next line
    bool foundBrace = false;
    if (!tokens.empty() && tokens.back() == "{")
    {
        foundBrace = true;
    }
    std::string line;
    if (!foundBrace)
    {
        if (!std::getline(file, line)) return;
        std::istringstream brace_iss(line);
        std::string maybeBrace;
        if (!(brace_iss >> maybeBrace) || maybeBrace != "{") return;
    }
    while (std::getline(file, line))
    {
        size_t closePos = line.find('}');
        bool blockEnd = (closePos != std::string::npos);
        std::string lineContent = blockEnd ? line.substr(0, closePos) : line;

        std::istringstream liss(lineContent);
        std::string lkey;
        if (!(liss >> lkey))
        {
            if (blockEnd) break;
            continue;
        }
    	validateDirective(LOCATION_DIRECTIVES, LOCATION_DIRECTIVES_COUNT, lkey);
        std::vector<std::string> ltokens = readValues(liss);
        parseCommonConfigField(loc, lkey, ltokens);
        parseLocationConfigField(loc, lkey, ltokens);
        if (blockEnd) break;
    }
    // Check for duplicate location paths
	for (size_t i = 0; i < config.locations.size(); ++i)
    {
    if (config.locations[i].path == loc.path)
        throw ConfigParseException("Duplicate location path: " + loc.path);
    }

    config.locations.push_back(loc);
            inLocationBlock = false;
}


// Parsing of the location-specific config fields
void Config::parseLocationConfigField(LocationConfig& config, const std::string& key, const std::vector<std::string>& tokens) {
	if (!tokens.empty())
  	{
		if (key == "upload_enabled")
   			parseUploadEnabled(config, tokens);
		else if (key == "upload_store")
   			parseUploadStore(config, tokens);
		else if (key == "redirect")
    		parseRedirect(config, tokens);
	}
}

// Parsing of the server-specific config fields
void Config::parseServerConfigField(ConfigData& config, const std::string& key, const std::vector<std::string>& tokens, std::ifstream& file)
{
  // Validate directive based on context
    if (!inLocationBlock)
        validateDirective(SERVER_DIRECTIVES, SERVER_DIRECTIVES_COUNT, key);
    if (tokens.empty())
        throw ConfigParseException("Directive " + key + " requires at least one argument");
    if (key == "location")
        parseLocationBlock(config, file, tokens);
    else if (key == "listen")
        parseListenDirective(config, tokens[0]);
    else if (key == "server_name")
        addUnique(config.server_names, tokens[0]);
    else if (key == "backlog")
        parseBacklogDirective(config, tokens[0]);
    else if (key == "keepalive_timeout")
        parseKeepaliveTimeoutDirective(config, tokens[0]);
    else if (key == "keepalive_max_requests")
        parseKeepaliveRequestsDirective(config, tokens[0]);
    else if (key == "error_log")
        assignLogFile(config.error_log, tokens[0]);
    else if (key == "access_log")
        assignLogFile(config.access_log, tokens[0]);
}


void	Config::strictCheckAfterServerBlock(std::ifstream& file, std::string line)
{
// --- Strict check for extra braces or invalid tokens ---
    //so the meaning is that after parsing the server block we
    // save the current position then parse "server" tocken and
    // if parsed correctly go back to the position right before
    // the "server" to parse this block, if not then throw exception
    std::streampos pos = file.tellg(); // save current position
    while (std::getline(file, line))
    {
        std::istringstream checkIss(line);
        std::string checkToken;
        if (!(checkIss >> checkToken))
        	continue; // skip empty lines
        if (checkToken == "server")
        {
            file.clear(); // clear any EOF or fail flags
            file.seekg(pos); // restores the position for the next server block
            break; // allow next server
        }
            throw ConfigParseException("Unexpected token after server block: " + checkToken);
    }
}


// Loads configuration data from a file at the given path.
// Returns true if the file was successfully read and parsed, false otherwise

// src/config/config.cpp

bool Config::parseConfigFile(std::ifstream& file)
{
    std::string line;
    int braceCount = 0;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string token;
        while (iss >> token)
        {
            if (token == "server")
          	{
                // Expect '{' after 'server'
                std::string nextToken;
                if (!(iss >> nextToken))
                {
                    if (!std::getline(file, line))
                        throw ConfigParseException("Expected '{' after server");
                    std::istringstream braceIss(line);
                    if (!(braceIss >> nextToken) || nextToken != "{")
                        throw ConfigParseException("Expected '{' after server");
                }
                else if (nextToken != "{")
                    throw ConfigParseException("Expected '{' after server");
                ConfigData serverConfig;
                braceCount = 1;
                // Parse server block
                while (braceCount > 0 && std::getline(file, line))
                {
                    std::istringstream blockIss(line);
                    std::string blockToken;
                    while (blockIss >> blockToken)
                    {
                        if (blockToken == "{")
                        {
                      		braceCount++;
                        	continue;
                        }
                        if (blockToken == "}")
                        {
                            braceCount--;
                            if (braceCount < 0)
                                throw ConfigParseException("Unexpected closing brace in config file");
                            continue;
                        }
                        std::vector<std::string> tokens = readValues(blockIss);
                        parseServerConfigField(serverConfig, blockToken, tokens, file);
                        parseCommonConfigField(serverConfig, blockToken, tokens);
                    }
                }
                validateConfig(serverConfig);
                _servers.push_back(serverConfig);
                if (braceCount != 0)
                    throw ConfigParseException("Mismatched braces in config file");
            	strictCheckAfterServerBlock(file, line);
            }
        }
    	if (_servers.empty())
        	throw ConfigParseException("No server blocks found in config file");
	}
return true;
}

bool Config::parseConfig(char *argv){
  std::string configPath = "conf/" + std::string(argv);
    std::ifstream file(configPath.c_str());
    if (!file.is_open())
        throw ConfigParseException("Failed to open config file: " + configPath);
    parseConfigFile(file);
    return true;
}
