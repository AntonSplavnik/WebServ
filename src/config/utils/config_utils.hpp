#ifndef CONFIG_UTILS_HPP
#define CONFIG_UTILS_HPP

#include <string>
#include <vector>
#include <algorithm>

// Valid CGI extensions
static const char* VALID_CGI_EXTENSIONS[] = {
	".py", ".php"
};
static const size_t VALID_CGI_EXTENSIONS_COUNT = sizeof(VALID_CGI_EXTENSIONS) / sizeof(VALID_CGI_EXTENSIONS[0]);

// Valid HTTP methods
static const char* HTTP_METHODS[] = {"GET", "POST", "DELETE"};
static const size_t HTTP_METHODS_COUNT = sizeof(HTTP_METHODS) / sizeof(HTTP_METHODS[0]);

// Path helpers
std::string normalizePath(const std::string& path);
bool isValidFile(const std::string& path, int mode);
bool isValidPath(const std::string& path, int mode);

// Validation helpers
bool isValidHttpMethod(const std::string& method);
bool isValidHttpStatusCode(int code);
bool isValidIPv4(const std::string& ip);
bool isValidHost(const std::string& host);
bool isValidCgiExt(const std::string& ext);
bool isValidAutoindexValue(const std::string& value);

// Vector utility - add unique element
template<typename T>
void addUnique(std::vector<T>& dest, const T& value) {
	if (std::find(dest.begin(), dest.end(), value) == dest.end()) {
		dest.push_back(value);
	}
}

#endif // CONFIG_UTILS_HPP
