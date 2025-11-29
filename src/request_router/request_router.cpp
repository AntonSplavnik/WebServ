/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request_router.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/21 17:43:54 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/29 22:35:10 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "request_router.hpp"
#include "connection.hpp"

static RoutingResult prepErrorResult(RoutingResult& result, bool success, int errorCode) {
	result.success = success;
	result.errorCode = errorCode;
	return result;
}

RoutingResult RequestRouter::route(Connection& connection) {

	const HttpRequest& req = connection.getRequest();
	RoutingResult result;

	// Find server config (virtual hosting)
	ConfigData& serverConfig = findServerConfig(req, connection.getServerPort());
	result.serverConfig = &serverConfig;

	// Find matching location
	const LocationConfig* location = serverConfig.findMatchingLocation(req.getPath());
	if (!location) {
		return prepErrorResult(result, false, 404);
	} else {
		result.location = location;
	}

	// Validate method
	if (!validateMethod(req, location)) {
		return prepErrorResult(result, false, 405);
	}

	// Validate body size BEFORE reading
	if (!validateBodySize(connection.getRequest().getContentLength(), location)) {
		return prepErrorResult(result, false, 413);
	}

	// Map path
	std::string mappedPath = mapPath(req, location);

	// Validate security
	if (!validatePathSecurity(mappedPath, location->root)) {
		return prepErrorResult(result, false, 403);
	}
	result.mappedPath = mappedPath;

	// Classify request type
	RequestType type = classify(req, location);
	result.type = type;

	result.success = true;
	return result;
}

ConfigData& RequestRouter::findServerConfig(const HttpRequest& req, int servrPort) {

	// Filter servers by port
	std::vector<ConfigData*> matchedConfigs;

	for (size_t i = 0; i < _configs.size(); i++) {
		for (size_t j = 0; j < _configs[i].listeners.size(); j++) {
			if (_configs[i].listeners[j].second == servrPort) {
				matchedConfigs.push_back(&_configs[i]);
				break;
			}
		}
	}

	// Extract Host header
	const std::map<std::string, std::string>& headers = req.getHeaders();
	std::map<std::string, std::string>::const_iterator it = headers.find("host");

	std::string hostValue;
	if (it != headers.end()) {
		hostValue = it->second;

		// Strip port from Host header (e.g., "example.com:8080" -> "example.com")
		size_t colonPos = hostValue.find(':');
		if (colonPos != std::string::npos) {
			hostValue = hostValue.substr(0, colonPos);
		}
	}

	// Match against server_names
	for (size_t i = 0; i < matchedConfigs.size(); i++) {
		const std::vector<std::string>& serverNames = matchedConfigs[i]->server_names;

		for (size_t j = 0; j < serverNames.size(); j++) {
			if (serverNames[j] == hostValue) {
				return *matchedConfigs[i];  // Found match!
			}
		}
	}

	// No match - return first server as default (nginx behavior)
	if (matchedConfigs.empty()) {
		std::cerr << "[FATAL] No config for port " << servrPort << std::endl;
		exit(1);  // Die loudly so bug is found
	}
	return *matchedConfigs[0];
}
bool RequestRouter::validateMethod(const HttpRequest& request, const LocationConfig*& location) {

	bool methodAllowed = false;
	for (size_t i = 0; i < location->allow_methods.size(); ++i) {
		if (location->allow_methods[i] == request.getMethod()) {
			methodAllowed = true;
			break;
		}
	}

	if (!methodAllowed) {
		std::cout << "[DEBUG] Method " << request.getMethod() << " not allowed for this location" << std::endl;
		return false;
	}

	return true;
}
bool RequestRouter::validateBodySize(int contentLength, const LocationConfig*& location) {


	if (contentLength > location->client_max_body_size) {
		std::cout << "[DEBUG] Body size " << contentLength
				  << " exceeds limit " << location->client_max_body_size << std::endl;
		return false;
	}

	return true;
}
std::string RequestRouter::mapPath(const HttpRequest& request, const LocationConfig*& matchedLocation) {

	std::string locationRoot = matchedLocation->root;
	std::string locationPath = matchedLocation->path;
	std::string requestPath = request.getPath();
	std::string relativePath;

	if(requestPath.compare(0, locationPath.length(), locationPath) == 0)
		relativePath = requestPath.substr(locationPath.length());
	else{
		std::cerr << "[ERROR] mapPath called with non-matching paths!" << std::endl;
		relativePath = requestPath;  // Fallback
	}

	if(!locationRoot.empty() && locationRoot[locationRoot.length() - 1 ] == '/'
		&& !relativePath.empty() && relativePath[0] == '/')
		relativePath = relativePath.substr(1);

	std::cout << "[DEBUG] LocationRoot: " << locationRoot << std::endl;
	std::cout << "[DEBUG] LocationPath: " << locationPath << std::endl;
	std::cout << "[DEBUG] RequestPath : " << requestPath << std::endl;
	std::cout << "[DEBUG] RelativePath : " << relativePath << std::endl;
	std::cout << "[DEBUG] MappedPath : " << locationRoot + relativePath << std::endl;
	return locationRoot + relativePath;
}
bool RequestRouter::validatePathSecurity(const std::string& mappedPath, const std::string& allowedRoot) {

	if(mappedPath.find("../") != std::string::npos || mappedPath.find("/..") != std::string::npos) {
		std::cout << "[SECURITY] Path traversal attempt detected: " << mappedPath << std::endl;
		return false;
	}

	if(mappedPath.find('\0') != std::string::npos){
		std::cout << "[SECURITY] Null byte injection detected" << std::endl;
		return false;
	}

	// Canonical path check (strongest defense)
	char resolvedPath[PATH_MAX];
	char resolvedRoot[PATH_MAX];

	// Resolve the mapped path to canonical form
	// Note: realpath() returns NULL if file doesn't exist yet (important for POST/upload)
	if (realpath(mappedPath.c_str(), resolvedPath) == NULL) {

		// File doesn't exist - for POST this is expected
		// Walk up directory tree to find existing parent
		std::string checkPath = mappedPath;
		std::string remainder = "";

		while (realpath(checkPath.c_str(), resolvedPath) == NULL) {
			size_t lastSlash = checkPath.find_last_of('/');
			if (lastSlash == std::string::npos) {
				std::cout << "[SECURITY] Invalid path or parent directory: " << mappedPath << std::endl;
				return false;
			}

			// Save the part we're removing
			remainder = checkPath.substr(lastSlash) + remainder;
			checkPath = checkPath.substr(0, lastSlash);

			// Avoid infinite loop if we reach empty path
			if (checkPath.empty()) {
				std::cout << "[SECURITY] Invalid path or parent directory: " << mappedPath << std::endl;
				return false;
			}
		}

		// Reconstruct full path with the non-existing parts
		std::string fullPath = std::string(resolvedPath) + remainder;
		strncpy(resolvedPath, fullPath.c_str(), PATH_MAX - 1);
		resolvedPath[PATH_MAX - 1] = '\0';
	}

	// Resolve the allowed root
	if (realpath(allowedRoot.c_str(), resolvedRoot) == NULL) {
		std::cout << "[SECURITY] Invalid allowed root: " << allowedRoot << std::endl;
		return false;
	}

	// Check if resolved path starts with resolved root
	std::string canonicalPath(resolvedPath);
	std::string canonicalRoot(resolvedRoot);

	if (canonicalPath.compare(0, canonicalRoot.length(), canonicalRoot) != 0) {
		std::cout << "[SECURITY] Path escape attempt!" << std::endl;
		std::cout << "[SECURITY] Requested: " << mappedPath << std::endl;
		std::cout << "[SECURITY] Resolved:  " << canonicalPath << std::endl;
		std::cout << "[SECURITY] Root:      " << canonicalRoot << std::endl;
		return false;
	}
	return true;
}
RequestType RequestRouter::classify(const HttpRequest& req, const LocationConfig* location) {

	// Check for redirect
	if (!location->redirect.empty()) {
		return REDIRECT;
	}

	// Check for CGI
	// Check if path matches CGI extension
	std::string path = req.getPath();
	bool isCGI = false;

	// Check against cgi_extension list
	for (size_t i = 0; i < location->cgi_ext.size(); i++) {
		std::string ext = location->cgi_ext[i];

		// Check if path ends with this extension
		if (path.size() > ext.size() && path.substr(path.length() - ext.length()) == ext) {
				isCGI = true;
				break;
			}
	}
	const std::string& method = req.getMethod();

	if (isCGI) {
		if (method == "GET") {
			return CGI_GET;
		} else if (method == "POST") {
			return CGI_POST;
		}
		// DELETE on CGI? Treat as regular DELETE
	}

	// Classify by method
	if (method == "DELETE")  return DELETE;
	else if (method == "POST") return POST;
	return GET;
}
