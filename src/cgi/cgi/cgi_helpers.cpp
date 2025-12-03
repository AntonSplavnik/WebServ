#include "cgi.hpp"
#include <cerrno>
#include <cstring>

bool Cgi::chdirToScriptDir(const std::string& mappedPath) {
	std::string::size_type slash = mappedPath.find_last_of('/');
	std::string scriptDir = (slash != std::string::npos) ? mappedPath.substr(0, slash) : ".";

	if (chdir(scriptDir.c_str()) != 0) {
		std::string err = std::string("[CGI] chdir() failed for directory: ") + scriptDir + " — " + strerror(errno);
		//Logger::getInstance().logError("ERROR", err); TODO: use my logger
		return false;
	}
	return true;
}

std::string Cgi::extractCgiExtension(const std::string& path, const LocationConfig* location) {
	for (size_t i = 0; i < location->cgi_ext.size(); i++) {
		std::string ext = location->cgi_ext[i];
		// Check if path ends with extension (for simple cases like /script.py)
		if (path.size() > ext.size() && path.substr(path.length() - ext.length()) == ext) {
			return ext;
		}
		// Check if extension appears at end of a filename (for PATH_INFO cases like /script.py/path/info)
		size_t extPos = path.find(ext);
		if (extPos != std::string::npos) {
			size_t afterExt = extPos + ext.length();
			// Extension is at end of filename if followed by '/' or end of string
			if (afterExt >= path.length() || path[afterExt] == '/') {
				return ext;
			}
		}
	}
	return "";
}
