#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>
#include <string>
#include <ctime>

class Logger {
public:
    Logger(const std::string& accessPath, const std::string& errorPath);
    void logAccess(const std::string& clientIp, const std::string& method,
                   const std::string& url, int status, size_t size);
    void logError(const std::string& level, const std::string& message);

private:
    std::ofstream _accessLog;
    std::ofstream _errorLog;
    std::string getTimestamp() const;
};

#endif

