/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request_router.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/21 17:43:54 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/30 21:21:34 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "request_router.hpp"
#include "connection.hpp"
#include <climits>
#include "cgi.hpp"

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

    // Validate CGI or not
	std::string ext = Cgi::extractCgiExtension(req.getPath(), location);

	if (!ext.empty()) {
		// ======== CGI REQUEST ==========
		result.type = (req.getMethod() == "POST" ? CGI_POST : CGI_GET);
		result.cgiExtension = ext;

		// calculate relativePath like mapPath()
		std::string locPath = location->path;
		std::string requestPath = req.getPath();
		std::string relativePath;

		if (requestPath.compare(0, locPath.length(), locPath) == 0)
			relativePath = requestPath.substr(locPath.length());
		else
			relativePath = requestPath;

		if (!relativePath.empty() && relativePath[0] == '/')
			relativePath = relativePath.substr(1);

		std::cout << "[DEBUG] [CGI] LocationRoot: " << location->root << std::endl;
		std::cout << "[DEBUG] [CGI] LocationPath: " << locPath << std::endl;
		std::cout << "[DEBUG] [CGI] RequestPath: " << requestPath << std::endl;
		std::cout << "[DEBUG] [CGI] RelativePath: " << relativePath << std::endl;
		std::cout << "[DEBUG] [CGI] Extension: " << ext << std::endl;

		// ----- SPLIT SCRIPT vs PATH_INFO -----
		// Find extension in relativePath - must be at end of a filename (before '/' or end of string)
		size_t extPos = std::string::npos;
		size_t searchStart = 0;
		
		while (true) {
			size_t found = relativePath.find(ext, searchStart);
			if (found == std::string::npos)
				break;
			
			// Check if extension is at end of filename (followed by '/' or end of string)
			size_t afterExt = found + ext.length();
			if (afterExt >= relativePath.length() || relativePath[afterExt] == '/') {
				extPos = found;
				break;
			}
			searchStart = found + 1;
		}

		if (extPos == std::string::npos) {
			std::cout << "[DEBUG] [CGI] EXT NOT FOUND in relativePath\n";
			result.errorCode = 404;
			return result;
		}

		size_t scriptEnd = extPos + ext.length();

		// Script part: "test_POST_GET.py"
		std::string scriptPart = relativePath.substr(0, scriptEnd);

		// Path info: "/kek/lol" (everything after the script filename)
		std::string pathInfo = "";
		if (scriptEnd < relativePath.length()) {
			pathInfo = relativePath.substr(scriptEnd);
			if (!pathInfo.empty() && pathInfo[0] != '/')
				pathInfo = "/" + pathInfo;
		}

		// BUILD ABSOLUTE SCRIPT PATH
		std::string scriptPath = location->root;

		if (scriptPath.back() != '/')
			scriptPath += "/";

		scriptPath += scriptPart;

		std::cout << "[DEBUG] [CGI] ScriptPart: " << scriptPart << std::endl;
		std::cout << "[DEBUG] [CGI] ScriptPath: " << scriptPath << std::endl;
		std::cout << "[DEBUG] [CGI] PathInfo: " << pathInfo << std::endl;
		std::cout << "[DEBUG] [CGI] MappedPath: " << scriptPath << std::endl;

		result.scriptPath = scriptPath;
		result.mappedPath = scriptPath;    // filesystem path for execve
		result.pathInfo = pathInfo;

		result.success = true;
		return result;
	}

	// ======== STATIC REQUEST ==========
	result.type = classify(req, location);
	result.mappedPath = mapPath(req, location);

	// Also compute pathInfo for static requests (portion after location prefix)
	{
		std::string locPath = location->path;
		std::string requestPath = req.getPath();
		std::string rel;
		if (requestPath.compare(0, locPath.length(), locPath) == 0)
			rel = requestPath.substr(locPath.length());
		else
			rel = "";
		if (!rel.empty() && rel[0] != '/') rel = std::string("/") + rel;
		if (rel.empty()) rel = "/";
		result.pathInfo = rel;
	}

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
    // PathInfo for debugging should show the relative part after the location prefix
    std::string debugPathInfo = relativePath;
    if (!debugPathInfo.empty() && debugPathInfo[0] != '/')
        debugPathInfo = std::string("/") + debugPathInfo;
    if (debugPathInfo.empty())
        debugPathInfo = "/";
    std::cout << "[DEBUG] PathInfo : " << debugPathInfo << std::endl;
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
