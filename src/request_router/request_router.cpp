/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request_router.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/21 17:43:54 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/12/03 21:51:47 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "request_router.hpp"
#include "connection.hpp"
#include "logger.hpp"
#include <climits>

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
	result.location = location;
	if (!location){
		return prepErrorResult(result, false, 404);
	}

	// Validate method
	if (!validateMethod(req, location)) {
		return prepErrorResult(result, false, 405);
	}

	// Validate body size BEFORE reading
	if (!validateBodySize(connection.getRequest().getContentLength(), location)) {
		return prepErrorResult(result, false, 413);
	}

	// Extract PATH_INFO for CGI requests
	std::string cleanedPath;
	result.pathInfo = extractPathInfo(req.getPath(), location, cleanedPath);

	// Map path (uses cleanedPath for CGI, original path for non-CGI)
	result.mappedPath = mapPath(cleanedPath, location);

	// Validate security
	if (!validatePathSecurity(result.mappedPath, location->root)) {
		return prepErrorResult(result, false, 403);
	}

	result.cgiExtension = extractCgiExtension(req.getPath(), location);
	result.scriptName = cleanedPath;
	result.pathTranslated = buildPathTranslated(location->root, result.pathInfo);

	logDebug("CGI Extension: " + result.cgiExtension);
	logDebug("Script Name: " + result.scriptName);
	logDebug("PATH_INFO: " + result.pathInfo);
	logDebug("PATH_TRANSLATED: " + result.pathTranslated);

	// Classify request type (use cgiExtension to detect CGI with PATH_INFO)
	result.type = classify(req, location, result.cgiExtension);

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
		logError("No config for port " + toString(servrPort));
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
		logDebug("Method " + request.getMethod() + " not allowed for this location");
		return false;
	}

	return true;
}
bool RequestRouter::validateBodySize(int contentLength, const LocationConfig*& location) {


	if (contentLength > location->client_max_body_size) {
		logDebug("Body size " + toString(contentLength) + " exceeds limit " + toString(location->client_max_body_size));
		return false;
	}

	return true;
}
std::string RequestRouter::mapPath(const std::string& requestPath, const LocationConfig*& matchedLocation) {

	std::string locationRoot = matchedLocation->root;
	std::string locationPath = matchedLocation->path;
	std::string relativePath;

	if(requestPath.compare(0, locationPath.length(), locationPath) == 0)
		relativePath = requestPath.substr(locationPath.length());
	else{
		logError("mapPath called with non-matching paths!");
		relativePath = requestPath;  // Fallback
	}

	if(!locationRoot.empty() && locationRoot[locationRoot.length() - 1 ] == '/'
		&& !relativePath.empty() && relativePath[0] == '/')
		relativePath = relativePath.substr(1);

	logDebug("LocationRoot: " + locationRoot);
	logDebug("LocationPath: " + locationPath);
	logDebug("RequestPath : " + requestPath);
	logDebug("RelativePath : " + relativePath);
	logDebug("MappedPath : " + locationRoot + relativePath);
	return locationRoot + relativePath;
}
bool RequestRouter::validatePathSecurity(const std::string& mappedPath, const std::string& allowedRoot) {

	if(mappedPath.find("../") != std::string::npos || mappedPath.find("/..") != std::string::npos) {
		logWarning("Path traversal attempt detected: " + mappedPath);
		return false;
	}

	if(mappedPath.find('\0') != std::string::npos){
		logWarning("Null byte injection detected");
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
				logWarning("Invalid path or parent directory: " + mappedPath);
				return false;
			}

			// Save the part we're removing
			remainder = checkPath.substr(lastSlash) + remainder;
			checkPath = checkPath.substr(0, lastSlash);

			// Avoid infinite loop if we reach empty path
			if (checkPath.empty()) {
				logWarning("Invalid path or parent directory: " + mappedPath);
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
		logWarning("Invalid allowed root: " + allowedRoot);
		return false;
	}

	// Check if resolved path starts with resolved root
	std::string canonicalPath(resolvedPath);
	std::string canonicalRoot(resolvedRoot);

	if (canonicalPath.compare(0, canonicalRoot.length(), canonicalRoot) != 0) {
		logWarning("Path escape attempt!");
		logWarning("Requested: " + mappedPath);
		logWarning("Resolved:  " + canonicalPath);
		logWarning("Root:      " + canonicalRoot);
		return false;
	}
	return true;
}
std::string RequestRouter::extractCgiExtension(const std::string& path, const LocationConfig* location) {
	for (size_t i = 0; i < location->cgi_ext.size(); i++) {
		std::string ext = location->cgi_ext[i];
		size_t extPos = path.find(ext);
		if (extPos != std::string::npos) {
			// Check if this is at a reasonable position (not just any substring)
			size_t afterExt = extPos + ext.length();
			if (afterExt == path.length() || path[afterExt] == '/') {
				return ext;
			}
		}
	}
	return "";
}
std::string RequestRouter::extractPathInfo(const std::string& requestPath, const LocationConfig* location, std::string& cleanedPath) {
	std::string ext = extractCgiExtension(requestPath, location);

	if (ext.empty()) {
		// Not a CGI request - return original path
		cleanedPath = requestPath;
		return "";
	}

	// Find extension position
	size_t extPos = requestPath.find(ext);
	if (extPos == std::string::npos) {
		cleanedPath = requestPath;
		return "";
	}

	size_t scriptEnd = extPos + ext.length();

	// Extract PATH_INFO if present
	if (scriptEnd < requestPath.length()) {
		cleanedPath = requestPath.substr(0, scriptEnd);
		return requestPath.substr(scriptEnd);
	}

	// No PATH_INFO
	cleanedPath = requestPath;
	return "";
}
std::string RequestRouter::buildPathTranslated(const std::string& root, const std::string& pathInfo) {
	if (pathInfo.empty()) {
		return "";
	}

	// Avoid double slash when concatenating root + pathInfo
	if (!root.empty() && root[root.length() - 1] == '/' && pathInfo[0] == '/') {
		return root + pathInfo.substr(1);
	}
	return root + pathInfo;
}
RequestType RequestRouter::classify(const HttpRequest& req, const LocationConfig* location, const std::string& cgiExtension) {

	// Check for redirect
	if (!location->redirect.empty()) {
		return REDIRECT;
	}

	// Check for CGI (using pre-extracted extension to handle PATH_INFO correctly)
	bool isCGI = !cgiExtension.empty();
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
