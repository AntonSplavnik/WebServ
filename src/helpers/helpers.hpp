// src/helpers/helpers.hpp
#pragma once
#include <string>
#include <vector>
#include "helpers.tpp"

// Valid CGI extensions
static const char* VALID_CGI_EXTENSIONS[] = { ".cgi", ".pl", ".py", ".php", ".sh", ".rb", ".js", ".asp", ".exe", ".bat", ".tcl", ".lua" };
static const size_t VALID_CGI_EXTENSIONS_COUNT = sizeof(VALID_CGI_EXTENSIONS) / sizeof(VALID_CGI_EXTENSIONS[0]);

// Valid autoindex values
static const char* AUTOINDEX_VALUES[] = { "on", "off", "true", "false", "1", "0" };
static const size_t AUTOINDEX_VALUES_COUNT = sizeof(AUTOINDEX_VALUES) / sizeof(AUTOINDEX_VALUES[0]);

// Valid HTTP methods
static const char* HTTP_METHODS[] = { "GET", "POST", "DELETE"};
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

// Vector utility
template<typename T>
void addUnique(std::vector<T>& dest, const T& value);
// Value reader
std::vector<std::string> readValues(std::istringstream& iss);

