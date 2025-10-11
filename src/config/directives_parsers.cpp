#include "config.hpp"
#include "../exceptions/config_exceptions.hpp"
#include "../helpers/helpers.hpp"
#include <sstream>
#include <unistd.h>

void Config::parseBacklogDirective(ConfigData& config, const std::string& value) {
    int backlog = 0;
    std::istringstream valStream(value);
    if (!(valStream >> backlog) || backlog < 1 || backlog > MAX_BACKLOG)
        throw ConfigParseException("Invalid backlog value: " + value);
    config.backlog = backlog;
}

void Config::parseMaxClientsDirective(ConfigData& config, const std::string& value) {
    int max_clients = 0;
    std::istringstream valStream(value);
    if (!(valStream >> max_clients) || max_clients < 1 || max_clients > 10000)
        throw ConfigParseException("Invalid max_clients value: " + value);
    config.max_clients = max_clients;
}

void Config::parseKeepaliveTimeoutDirective(ConfigData& config, const std::string& value) {
    int keepalive_timeout = 0;
    std::istringstream valStream(value);
    if (!(valStream >> keepalive_timeout) || keepalive_timeout < 0 || keepalive_timeout > 3600)
        throw ConfigParseException("Invalid keepalive_timeout value: " + value);
    config.keepalive_timeout = keepalive_timeout;
}

void Config::parseKeepaliveRequestsDirective(ConfigData& config, const std::string& value) {
    int keepalive_max_requests = 0;
    std::istringstream valStream(value);
    if (!(valStream >> keepalive_max_requests) || keepalive_max_requests < 1 || keepalive_max_requests > 10000)
        throw ConfigParseException("Invalid keepalive_max_requests value: " + value);
    config.keepalive_max_requests = keepalive_max_requests;
}

void Config::parseListenDirective(ConfigData& config, const std::string& value) {
    size_t colon = value.find(':');
    std::string host = "0.0.0.0";
    int port = 80; // default port
    if (colon != std::string::npos) {
        host = value.substr(0, colon);
        std::string portPart = value.substr(colon + 1);
        if (!isValidIPv4(host) && !isValidHost(host))
            throw ConfigParseException("Invalid host in listen directive: " + host);
        std::istringstream portStream(portPart);
        if (!(portStream >> port) || port < 1 || port > 65535)
            throw ConfigParseException("Invalid port in listen directive: " + portPart);
    }
    else if (isValidIPv4(value))
    {
        host = value;
        port = 80;
    }
    else if (isValidHost(value))
    {
        host = value;
        port = 80;
    }
    else
        throw ConfigParseException("Invalid listen directive: " + value);
    std::pair<std::string,unsigned short> listenPair(host, static_cast<unsigned short>(port));
    if (std::find(config.listeners.begin(),
                  config.listeners.end(),
                  listenPair) != config.listeners.end()) {
        std::ostringstream oss;
        oss << port;
        throw ConfigParseException("Duplicate listen directive: " + host + ":" + oss.str());
    }
    config.listeners.push_back(std::make_pair(host, static_cast<unsigned short>(port)));
}

// In a suitable file, e.g., directivesParsers.cpp or config.cpp

void Config::parseUploadEnabled(LocationConfig& config, const std::vector<std::string>& tokens) {
    const std::string& val = tokens[0];
    if (val == "on" || val == "true" || val == "1") {
        config.upload_enabled = true;
    } else if (val == "off" || val == "false" || val == "0") {
        config.upload_enabled = false;
    } else {
        throw ConfigParseException("Invalid upload_enabled value: " + val);
    }
}

void Config::parseUploadStore(LocationConfig& config, const std::vector<std::string>& tokens) {
    if (!isValidPath(tokens[0], W_OK | X_OK))
        throw ConfigParseException("Invalid or inaccessible upload_store path: " + tokens[0]);
    config.upload_store = normalizePath(tokens[0]);
}

void Config::parseRedirect(LocationConfig& config, const std::vector<std::string>& tokens) {
    if (tokens.size() != 2)
        throw ConfigParseException("Redirect directive requires exactly 2 arguments: status code and target path/URL");
    int code = std::atoi(tokens[0].c_str());
    if (!isValidHttpStatusCode(code) || (code != 301 && code != 302 && code != 303))
        throw ConfigParseException("Invalid redirect status code: " + tokens[0]);
    config.redirect_code = code;
    config.redirect = tokens[1];
}
