#include "../config/config.hpp"
#include "../helpers/helpers.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <climits>
#include <sstream>

// Helper to read all values from iss, stripping trailing semicolons
std::vector<std::string> readValues(std::istringstream& iss) {
    std::vector<std::string> values;
    std::string value;
    while (iss >> value)
    {
        if (!value.empty() && value[value.size() - 1] == ';') value.erase(value.size() - 1);
        if (!value.empty()) values.push_back(value);
    }
    return values;
}

// Helper to normalize paths (resolve ., .., symlinks)
std::string normalizePath(const std::string& path) {
    char resolved[PATH_MAX];
    if (realpath(path.c_str(), resolved))
        return std::string(resolved);
    return path; // fallback if path does not exist
}

// Helper to validate if a path is a regular file and has the specified access mode
bool isValidFile(const std::string& path, int mode) {
    struct stat sb;
    if (stat(path.c_str(), &sb) != 0) return false;
    return S_ISREG(sb.st_mode) && (access(path.c_str(), mode) == 0);
}

// Helper to validate if a path is a directory and has the specified access mode
bool isValidPath(const std::string& path, int mode) {
    struct stat sb;
    if (stat(path.c_str(), &sb) != 0) return false;
    if (!S_ISDIR(sb.st_mode)) return false;
    return (access(path.c_str(), mode) == 0);
}

// Helper to validate HTTP methods
bool isValidHttpMethod(const std::string& method) {
    for (size_t i = 0; i < HTTP_METHODS_COUNT; ++i)
  	{
        if (method == HTTP_METHODS[i])
            return true;
    }
    return false;
}

// Helper to validate HTTP status codes
bool isValidHttpStatusCode(int code) {
    return code >= 100 && code <= 599;
}

// Helper to validate IPv4 addresses
bool isValidIPv4(const std::string& ip) {
    size_t start = 0, end = 0, count = 0;
    while (end != std::string::npos)
    {
        end = ip.find('.', start);
        std::string part = ip.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
        if (part.empty() || part.size() > 3) return false;
        for (size_t i = 0; i < part.size(); ++i)
            if (!isdigit(part[i])) return false;
        int num = std::atoi(part.c_str());
        if (num < 0 || num > 255) return false;
        start = end + 1;
        ++count;
    }
    return count == 4;
}

// Helper to validate host (simple check for IP or hostname)
bool isValidHost(const std::string& host) {
    if (isValidIPv4(host)) return true;
    if (host.empty()) return false;
    for (size_t i = 0; i < host.size(); ++i)
    {
        char c = host[i];
        if (!(isalnum(c) || c == '-' || c == '.')) return false;
    }
    // Reject all-numeric hostnames
    if (std::find_if(host.begin(), host.end(), ::isalpha) == host.end())
        return false;
    return true;
}

// Helper to validate CGI extensions
bool isValidCgiExt(const std::string& ext) {
    for (size_t i = 0; i < VALID_CGI_EXTENSIONS_COUNT; ++i)
  	{
        if (ext == VALID_CGI_EXTENSIONS[i])
            return true;
    }
    return false;
}

// Helper to validate autoindex values
bool isValidAutoindexValue(const std::string& value) {
    for (size_t i = 0; i < AUTOINDEX_VALUES_COUNT; ++i)
  	{
        if (value == AUTOINDEX_VALUES[i])
            return true;
    }
    return false;
}

// Helper to join two paths correctly
std::string joinPath(const std::string& base, const std::string& subpath) {
    if (!base.empty() && !subpath.empty()) {
        if (base[base.size() - 1] == '/' && subpath[0] == '/')
            return base + subpath.substr(1);
        else if (base[base.size() - 1] != '/' && subpath[0] != '/')
            return base + "/" + subpath;
        else
            return base + subpath;
    }
    return base + subpath;
}
