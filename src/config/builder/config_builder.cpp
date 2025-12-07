#include "config_builder.hpp"
#include "../utils/config_utils.hpp"
#include <sstream>
#include <cstdlib>

ConfigBuilder::ConfigBuilder() {}

std::vector<ConfigData> ConfigBuilder::build(const std::vector<Directive*>& tree) {
	std::vector<ConfigData> servers;

	for (size_t i = 0; i < tree.size(); ++i) {
		if (tree[i]->name == "server") {
			servers.push_back(buildServer(tree[i]));
		}
	}

	return servers;
}

ConfigData ConfigBuilder::buildServer(const Directive* server) {
	ConfigData config;

	// Process all server directives
	for (size_t i = 0; i < server->children.size(); ++i) {
		const Directive* dir = server->children[i];

		if (dir->name == "location") {
			config.locations.push_back(buildLocation(dir));
		} else {
			applyServerDirective(config, dir);
		}
	}

	return config;
}

LocationConfig ConfigBuilder::buildLocation(const Directive* location) {
	LocationConfig config;

	// Set location path
	config.path = location->getParam(0);

	// Process all location directives
	for (size_t i = 0; i < location->children.size(); ++i) {
		applyLocationDirective(config, location->children[i]);
	}

	return config;
}

void ConfigBuilder::applyServerDirective(ConfigData& config, const Directive* dir) {
	if (dir->name == "listen") {
		handleListen(config, dir);
	} else if (dir->name == "server_name") {
		handleServerName(config, dir);
	} else if (dir->name == "backlog") {
		handleBacklog(config, dir);
	} else if (dir->name == "keepalive_timeout") {
		handleKeepaliveTimeout(config, dir);
	} else if (dir->name == "keepalive_max_requests") {
		handleKeepaliveMaxRequests(config, dir);
	} else if (dir->name == "root") {
		handleRoot(config, dir);
	} else if (dir->name == "index") {
		handleIndex(config, dir);
	} else if (dir->name == "autoindex") {
		handleAutoindex(config, dir);
	} else if (dir->name == "allow_methods") {
		handleAllowMethods(config, dir);
	} else if (dir->name == "error_page") {
		handleErrorPage(config, dir);
	} else if (dir->name == "client_max_body_size") {
		handleClientMaxBodySize(config, dir);
	} else if (dir->name == "cgi_path") {
		handleCgiPath(config, dir);
	} else if (dir->name == "cgi_ext") {
		handleCgiExt(config, dir);
	}
}

void ConfigBuilder::applyLocationDirective(LocationConfig& config, const Directive* dir) {
	if (dir->name == "root") {
		handleRoot(config, dir);
	} else if (dir->name == "index") {
		handleIndex(config, dir);
	} else if (dir->name == "autoindex") {
		handleAutoindex(config, dir);
	} else if (dir->name == "allow_methods") {
		handleAllowMethods(config, dir);
	} else if (dir->name == "error_page") {
		handleErrorPage(config, dir);
	} else if (dir->name == "client_max_body_size") {
		handleClientMaxBodySize(config, dir);
	} else if (dir->name == "cgi_path") {
		handleCgiPath(config, dir);
	} else if (dir->name == "cgi_ext") {
		handleCgiExt(config, dir);
	} else if (dir->name == "upload_enabled") {
		handleUploadEnabled(config, dir);
	} else if (dir->name == "upload_store") {
		handleUploadStore(config, dir);
	} else if (dir->name == "redirect") {
		handleRedirect(config, dir);
	}
}

// --- Server-specific handlers ---

void ConfigBuilder::handleListen(ConfigData& config, const Directive* dir) {
	std::string value = dir->getParam(0);
	size_t colon = value.find(':');
	std::string host = "0.0.0.0";
	int port = 80;

	if (colon != std::string::npos) {
		host = value.substr(0, colon);
		std::string portPart = value.substr(colon + 1);
		std::istringstream portStream(portPart);
		portStream >> port;
	} else {
		// Could be just IP or just port
		std::istringstream iss(value);
		if (iss >> port) {
			// It's a port number
			host = "0.0.0.0";
		} else {
			// It's a hostname/IP
			host = value;
			port = 80;
		}
	}

	config.listeners.push_back(std::make_pair(host, static_cast<unsigned short>(port)));
}

void ConfigBuilder::handleServerName(ConfigData& config, const Directive* dir) {
	for (size_t i = 0; i < dir->parameters.size(); ++i) {
		config.server_names.push_back(dir->parameters[i]);
	}
}

void ConfigBuilder::handleBacklog(ConfigData& config, const Directive* dir) {
	std::istringstream iss(dir->getParam(0));
	iss >> config.backlog;
}

void ConfigBuilder::handleKeepaliveTimeout(ConfigData& config, const Directive* dir) {
	std::istringstream iss(dir->getParam(0));
	iss >> config.keepalive_timeout;
}

void ConfigBuilder::handleKeepaliveMaxRequests(ConfigData& config, const Directive* dir) {
	std::istringstream iss(dir->getParam(0));
	iss >> config.keepalive_max_requests;
}

// --- Location-specific handlers ---

void ConfigBuilder::handleUploadEnabled(LocationConfig& config, const Directive* dir) {
	std::string value = dir->getParam(0);
	config.upload_enabled = (value == "on" || value == "true" || value == "1");
}

void ConfigBuilder::handleUploadStore(LocationConfig& config, const Directive* dir) {
	config.upload_store = normalizePath(dir->getParam(0));
}

void ConfigBuilder::handleRedirect(LocationConfig& config, const Directive* dir) {
	std::istringstream iss(dir->getParam(0));
	iss >> config.redirect_code;
	config.redirect = dir->getParam(1);
}
