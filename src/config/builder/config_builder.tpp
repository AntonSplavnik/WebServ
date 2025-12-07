#pragma once

#include "../utils/config_utils.hpp"
#include <sstream>

// Template implementations for common directive handlers

template<typename ConfigT>
void ConfigBuilder::handleRoot(ConfigT& config, const Directive* dir) {
	std::string root = normalizePath(dir->getParam(0));
	// Ensure trailing slash
	if (!root.empty() && root[root.length() - 1] != '/') {
		root += '/';
	}
	config.root = root;
}

template<typename ConfigT>
void ConfigBuilder::handleIndex(ConfigT& config, const Directive* dir) {
	config.index = dir->getParam(0);
}

template<typename ConfigT>
void ConfigBuilder::handleAutoindex(ConfigT& config, const Directive* dir) {
	std::string value = dir->getParam(0);
	config.autoindex = (value == "on" || value == "true" || value == "1");
}

template<typename ConfigT>
void ConfigBuilder::handleAllowMethods(ConfigT& config, const Directive* dir) {
	for (size_t i = 0; i < dir->parameters.size(); ++i) {
		config.allow_methods.push_back(dir->parameters[i]);
	}
}

template<typename ConfigT>
void ConfigBuilder::handleErrorPage(ConfigT& config, const Directive* dir) {
	// Last parameter is the file path
	std::string path = dir->parameters.back();

	// All other parameters are status codes
	for (size_t i = 0; i < dir->parameters.size() - 1; ++i) {
		int code = 0;
		std::istringstream iss(dir->parameters[i]);
		if (iss >> code) {
			config.error_pages[code] = path;
		}
	}
}

template<typename ConfigT>
void ConfigBuilder::handleClientMaxBodySize(ConfigT& config, const Directive* dir) {
	std::istringstream iss(dir->getParam(0));
	unsigned long size = 0;
	iss >> size;
	config.client_max_body_size = size;
}

template<typename ConfigT>
void ConfigBuilder::handleCgiPath(ConfigT& config, const Directive* dir) {
	for (size_t i = 0; i < dir->parameters.size(); ++i) {
		config.cgi_path.push_back(dir->parameters[i]);
	}
}

template<typename ConfigT>
void ConfigBuilder::handleCgiExt(ConfigT& config, const Directive* dir) {
	for (size_t i = 0; i < dir->parameters.size(); ++i) {
		config.cgi_ext.push_back(dir->parameters[i]);
	}
}
