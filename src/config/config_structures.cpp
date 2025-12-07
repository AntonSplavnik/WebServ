#include "config_structures.hpp"
#include "utils/config_utils.hpp"
#include <sstream>

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
	: listeners(),
	  server_names(),
	  root(""),
	  index(""),
	  autoindex(false),
	  backlog(0),
	  keepalive_timeout(15),
	  keepalive_max_requests(100),
	  allow_methods(),
	  error_pages(),
	  client_max_body_size(0),
	  cgi_path(),
	  cgi_ext(),
	  locations() {}

const LocationConfig* ConfigData::findMatchingLocation(const std::string& requestPath) const {
	const LocationConfig* bestMatch = NULL;
	size_t longestMatch = 0;

	for (size_t i = 0; i < locations.size(); ++i) {
		size_t pathLen = locations[i].path.length();

		// Check if request path starts with this location path
		if (requestPath.compare(0, pathLen, locations[i].path) == 0) {
			// For root location "/", always matches
			if (locations[i].path == "/") {
				if (pathLen > longestMatch) {
					bestMatch = &locations[i];
					longestMatch = pathLen;
				}
			}
			// For non-root locations, ensure proper path boundary
			else if (requestPath.length() == pathLen || requestPath[pathLen] == '/') {
				if (pathLen > longestMatch) {
					bestMatch = &locations[i];
					longestMatch = pathLen;
				}
			}
		}
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
