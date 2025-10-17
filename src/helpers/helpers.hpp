// src/helpers/helpers.hpp
#pragma once
#include <string>
#include <vector>
#include "helpers.tpp"

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
// Value reader from a stream
std::vector<std::string> readValues(std::istringstream& iss);
std::string joinPath(const std::string& base, const std::string& subpath);

