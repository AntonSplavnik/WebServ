#include "cgi.hpp"


bool Cgi::chdirToScriptDir(const std::string &scriptPath) {
	std::string::size_type slash = scriptPath.find_last_of('/');
	std::string scriptDir = (slash != std::string::npos) ? scriptPath.substr(0, slash) : ".";

	if (chdir(scriptDir.c_str()) != 0) {
		std::string err = std::string("[CGI] chdir() failed for directory: ") + scriptDir + " — " + strerror(errno);
		//Logger::getInstance().logError("ERROR", err); TODO: use my logger
		return false;
	}
	return true;
}
