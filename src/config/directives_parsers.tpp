#pragma once

#include "../exceptions/config_exceptions.hpp"
#include "../helpers/helpers.hpp"
#include "config.hpp"
#include <unistd.h>
#include <sstream>

template<typename ConfigT>
void Config::parseAutoindex(ConfigT& config, const std::vector<std::string>& tokens) {
    if (!isValidAutoindexValue(tokens[0]))
        throw ConfigParseException("Invalid autoindex value: " + tokens[0]);
    config.autoindex = (tokens[0] == "on" || tokens[0] == "true" || tokens[0] == "1");
}

template<typename ConfigT>
void Config::parseRoot(ConfigT& config, const std::vector<std::string>& tokens) {
    if (!config.root.empty())
        throw ConfigParseException("Duplicate root directive");
    if (!isValidPath(tokens[0], R_OK | X_OK))
        throw ConfigParseException("Invalid or inaccessible root path: " + tokens[0]);
    config.root = normalizePath(tokens[0]);
}

template<typename ConfigT>
void Config::parseIndex(ConfigT& config, const std::vector<std::string>& tokens) {
    if (!config.index.empty())
        throw ConfigParseException("Duplicate index directive");
    if (!isValidFile(tokens[0], R_OK))
        throw ConfigParseException("Invalid or inaccessible index file: " + tokens[0]);
    config.index = tokens[0];
}

template<typename ConfigT>
void Config::parseClientMaxBodySize(ConfigT& config, const std::vector<std::string>& tokens) {
    if (config.client_max_body_size > 0)
        throw ConfigParseException("Duplicate client_max_body_size directive");
    std::istringstream iss(tokens[0]);
    int size = 0;
    if (!(iss >> size))
        throw ConfigParseException("Invalid client_max_body_size value: " + tokens[0]);
    if (size <= 0)
        throw ConfigParseException("client_max_body_size must be positive: " + tokens[0]);
    if (static_cast<size_t>(size) > MAX_CLIENT_BODY_SIZE)
    {
        std::ostringstream oss;
        oss << MAX_CLIENT_BODY_SIZE;
        throw ConfigParseException("client_max_body_size (" + tokens[0] + ") exceeds maximum allowed (" + oss.str() + ")");
    }
    config.client_max_body_size = size;
}

template<typename ConfigT>
void Config::parseAllowMethods(ConfigT& config, const std::vector<std::string>& tokens) {
    for (size_t i = 0; i < tokens.size(); ++i)
  	{
        if (!isValidHttpMethod(tokens[i]))
            throw ConfigParseException("Invalid HTTP method in allow_methods: " + tokens[i]);
        addUnique(config.allow_methods, tokens[i]);
    }
}

template<typename ConfigT>
void Config::parseErrorPage(ConfigT& config, const std::vector<std::string>& tokens) {
    if (tokens.size() < 2)
      throw ConfigParseException("error_page directive requires at least one status code and a file path");
    std::string path = tokens.back();
    if (!isValidFile(path, R_OK))
        throw ConfigParseException("Invalid or inaccessible error_page file: " + path);
    for (size_t i = 0; i < tokens.size() - 1; ++i)
    {
        if (tokens[i].empty()) continue;
        int code = 0;
        std::istringstream codeStream(tokens[i]);
        if (codeStream >> code)
        {
            if (!isValidHttpStatusCode(code))
                throw ConfigParseException("Invalid HTTP status code in error_page: " + tokens[i]);
            config.error_pages[code] = path;
        }
    }
}

template<typename ConfigT>
void Config::parseCgiExt(ConfigT& config, const std::vector<std::string>& tokens) {
    for (size_t i = 0; i < tokens.size(); ++i)
  	{
        if (!isValidCgiExt(tokens[i]))
            throw ConfigParseException("Invalid CGI extension: " + tokens[i]);
        addUnique(config.cgi_ext, tokens[i]);
    }
}

template<typename ConfigT>
void Config::parseCgiPath(ConfigT& config, const std::vector<std::string>& tokens) {
    for (size_t i = 0; i < tokens.size(); ++i)
  {
        if (!isValidPath(tokens[i], X_OK))
            throw ConfigParseException("Invalid CGI path: " + tokens[i]);
        addUnique(config.cgi_path, tokens[i]);
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
