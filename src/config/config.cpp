#include "config.hpp"
#include "../helpers/helpers.hpp"
#include "../exceptions/config_exceptions.hpp"
#include "../logging/logger.hpp"
#include "directivesParsers.tpp"
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
      cgi_path(),
      cgi_ext(),
      client_max_body_size(0),
      upload_enabled(false),
      upload_store(""),
      redirect(""),
      redirect_code(0) {}

ConfigData::ConfigData()
    :
      server_names(),
      root(""),
      index(""),
      backlog(0),
      access_log(""),
      error_log(""),
      autoindex(),
      locations(),
      cgi_path(),
      cgi_ext(),
      error_pages(),
      allow_methods(),
      client_max_body_size(0) {}

std::vector<ConfigData> Config::getServers() const {
    return _servers;
}
// Validates that the given key is in the list of known directives
void Config::validateDirective(const char* const* directives, size_t count, const std::string& key) {
    if (std::find(directives, directives + count, key) == directives + count)
        throw ConfigParseException("Unknown directive: " + key);
}

// validates both minimum required directives and also applies default values,
// performs fallbacks, and checks for consistency and correctness of various
// configuration fields (including location-specific settings, CGI, uploads,
// and redirects). It ensures not only presence but also validity and proper
// relationships between directives.
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
		if (loc.path.empty() || loc.path[0] != '/')
    		throw ConfigParseException("Invalid location config: path must start with '/': " + loc.path);
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
        if (loc.cgi_ext.empty())
          loc.cgi_ext = config.cgi_ext;
        if (loc.cgi_path.empty())
          loc.cgi_path = config.cgi_path;
        if (!loc.cgi_ext.empty() && loc.cgi_path.empty())
            throw ConfigParseException("Missing required location config: cgi_path for CGI");
        if (!loc.cgi_path.empty() && loc.cgi_ext.empty())
            throw ConfigParseException("Missing required location config: cgi_ext for CGI");
        if (loc.error_pages.empty())
    		loc.error_pages = config.error_pages;
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

void Config::parseLocationBlock(ConfigData& config, std::ifstream& file, const std::vector<std::string>& tokens) {
  	inLocationBlock = true;
            LocationConfig loc;
            loc.path = tokens[0];

            // Check for '{' at end of line or next line
            bool foundBrace = false;
            if (!tokens.empty() && tokens.back() == "{") {
                foundBrace = true;
            }
            std::string line;
            if (!foundBrace) {
                if (!std::getline(file, line)) return;
                std::istringstream brace_iss(line);
                std::string maybeBrace;
                if (!(brace_iss >> maybeBrace) || maybeBrace != "{") return;
            }
    while (std::getline(file, line)) {
        size_t closePos = line.find('}');
        bool blockEnd = (closePos != std::string::npos);
        std::string lineContent = blockEnd ? line.substr(0, closePos) : line;

        std::istringstream liss(lineContent);
        std::string lkey;
        if (!(liss >> lkey)) {
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
	for (size_t i = 0; i < config.locations.size(); ++i) {
    if (config.locations[i].path == loc.path)
        throw ConfigParseException("Duplicate location path: " + loc.path);
    }

    config.locations.push_back(loc);
            inLocationBlock = false;
}


// Parsing of the location-specific config fields
void Config::parseLocationConfigField(LocationConfig& config, const std::string& key, const std::vector<std::string>& tokens) {
if (!tokens.empty()) {
    if (key == "upload_enabled")
        parseUploadEnabled(config, tokens);
    else if (key == "upload_store")
        parseUploadStore(config, tokens);
    else if (key == "redirect")
        parseRedirect(config, tokens);
}
}

// Helper to parse common config fields
template<typename ConfigT>
void Config::parseCommonConfigField(ConfigT& config, const std::string& key, const std::vector<std::string>& tokens) {
    if (tokens.empty())
      throw ConfigParseException("Directive " + key + " requires at least one argument");
    if (key == "autoindex")
        parseAutoindex(config, tokens);
    else if (key == "root")
        parseRoot(config, tokens);
    else if (key == "index")
        parseIndex(config, tokens);
    else if (key == "client_max_body_size")
        parseClientMaxBodySize(config, tokens);
    else if (key == "allow_methods")
        parseAllowMethods(config, tokens);
    else if (key == "cgi_ext")
        parseCgiExt(config, tokens);
    else if (key == "cgi_path")
        parseCgiPath(config, tokens);
    else if (key == "error_page")
        parseErrorPage(config, tokens);
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
		else if (key == "error_log")
    		assignLogFile(config.error_log, tokens[0]);
        else if (key == "access_log")
    		assignLogFile(config.access_log, tokens[0]);
	}

// Loads configuration data from a file at the given path.
// Returns true if the file was successfully read and parsed, false otherwise
bool Config::parseConfigFile(std::ifstream& file)
{
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key)) continue;
        if (key == "server") {
            // Expect '{' after 'server'
            std::string brace;
            if (!(iss >> brace) || brace != "{") {
                // Optionally, read next line for '{'
                if (!(std::getline(file, line))) throw ConfigParseException("Expected '{' after server");
                if (line.find('{') == std::string::npos) throw ConfigParseException("Expected '{' after server");
            }
            ConfigData serverConfig;
            // Parse server block
            int braceCount = 1;
            while (braceCount > 0 && std::getline(file, line)) {
                std::istringstream blockIss(line);
                std::string blockKey;
                if (!(blockIss >> blockKey)) continue;
                if (blockKey == "{") { braceCount++; continue; }
                if (blockKey == "}") {
    				braceCount--;
    				if (braceCount < 0)
        				throw ConfigParseException("Unexpected closing brace in config file");
    				continue;
				}
                std::vector<std::string> tokens = readValues(blockIss);
                parseServerConfigField(serverConfig, blockKey, tokens, file);
                parseCommonConfigField(serverConfig, blockKey, tokens);
            }
            validateConfig(serverConfig);
            _servers.push_back(serverConfig);
        }
}
    return !_servers.empty();
}

bool Config::parseConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw ConfigParseException("Failed to open config file: " + path);

    parseConfigFile(file);

    return true;
}