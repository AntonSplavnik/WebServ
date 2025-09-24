#include "../config/config.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <regex>
#include <sstream>

// Helper to read all values from iss, stripping trailing semicolons
std::vector<std::string> readValues(std::istringstream& iss) {
    std::vector<std::string> values;
    std::string value;
    while (iss >> value) {
        if (!value.empty() && value.back() == ';') value.pop_back();
        if (!value.empty()) values.push_back(value);
    }
    return values;
}

// Helper to normalize paths (resolve ., .., symlinks)
std::string normalizePath(const std::string& path) {
    char resolved[PATH_MAX];
    if (realpath(path.c_str(), resolved)) {
        return std::string(resolved);
    }
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
    static const char* valid[] = {
        "GET", "POST", "DELETE"
    };
    return std::find(valid, valid + 3, method) != (valid + 3);
}

// Helper to validate HTTP status codes
bool isValidHttpStatusCode(int code) {
    return code >= 100 && code <= 599;
}

// Helper to validate IPv4 addresses
bool isValidIPv4(const std::string& ip) {
    size_t start = 0, end = 0, count = 0;
    while (end != std::string::npos) {
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

// Helper to validate host (simple check for IP or hostname) TODO: make no regex
bool isValidHost(const std::string& host) {
    if (isValidIPv4(host)) return true;
    static const std::regex hostname("^([a-zA-Z0-9\\-\\.]+)$");
    if (std::regex_match(host, hostname)) {
        // Reject all-numeric hostnames
        if (std::find_if(host.begin(), host.end(), ::isalpha) == host.end())
            return false;
        return true;
    }
    return false;
}

// Helper to validate CGI extensions
bool isValidCgiExt(const std::string& ext) {
    static const char* valid[] = { ".cgi", ".pl", ".py", ".php", ".sh", ".rb", ".js", ".asp", ".exe", ".bat", ".tcl", ".lua" };
    return std::find(valid, valid + 12, ext) != (valid + 12);
}

// Helper to validate autoindex values
bool isValidAutoindexValue(const std::string& value) {
    return value == "on" || value == "off" ||
           value == "true" || value == "false" ||
           value == "1" || value == "0";
}


