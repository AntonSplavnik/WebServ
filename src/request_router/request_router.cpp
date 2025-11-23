#include "request_router.hpp"
#include <sys/stat.h>
#include <cerrno>

bool RequestRouter::validateMethod(const HttpRequest& request, const LocationConfig* location) {

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
		// Validate the parent directory instead
		std::string parentDir = mappedPath.substr(0, mappedPath.find_last_of('/'));
		if (parentDir.empty()) parentDir = ".";

		if (realpath(parentDir.c_str(), resolvedPath) == NULL) {
			std::cout << "[SECURITY] Invalid path or parent directory: " << mappedPath << std::endl;
			return false;
		}
		// Add back the filename for comparison
		std::string filename = mappedPath.substr(mappedPath.find_last_of('/'));
		std::string fullPath = std::string(resolvedPath) + filename;
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

bool RequestRouter::getPathInfo(const std::string& path, RequestType type, struct stat* statBuf) {
	// For POST/UPLOAD, skip existence check (may create new files)
	if (type == UPLOAD) {
		return false; // Don't check, let handler deal with it
	}
	
	// For GET/DELETE, check if path exists
	if (stat(path.c_str(), statBuf) != 0) {
		return false; // Path doesn't exist or error
	}
	
	return true; // Path exists
}
