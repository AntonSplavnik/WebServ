// Logger.cpp
#include "logger.hpp"

//Instructions: Implement a simple logging system that writes access and error logs to specified files.
//usage:
//Log with logAccess whenever you successfully handle an HTTP request—record every client request, regardless of outcome (even for 404 or 500 responses).
//Use logError for server-side issues, misconfigurations, internal errors, or unexpected conditions—anything that is not a normal request, such as failed file access, config errors, or exceptions


// Initialize log files
Logger::Logger(const std::string& accessPath, const std::string& errorPath)
    : _accessLog(accessPath.c_str(), std::ios::app),
      _errorLog(errorPath.c_str(), std::ios::app) {}

// Get current timestamp in YYYY-MM-DD HH:MM:SS format
std::string Logger::getTimestamp() const {
    time_t now = time(0);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return buf;
}

// Log an access entry. Format: [timestamp] clientIp "METHOD URL" status size
void Logger::logAccess(const std::string& clientIp, const std::string& method,
                       const std::string& url, int status, size_t size) {
    _accessLog << getTimestamp() << " " << clientIp << " \"" << method << " "
               << url << "\" " << status << " " << size << std::endl;
}

// Log an error entry. Format: [timestamp] [level](INFO, WARNING, or ERROR) message
void Logger::logError(const std::string& level, const std::string& message) {
    _errorLog << getTimestamp() << " [" << level << "] " << message << std::endl;
}