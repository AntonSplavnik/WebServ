/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/05 00:00:00 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/12/05 00:00:00 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <sstream>

enum LogLevel {
	LOG_DEBUG = 0,
	LOG_INFO = 1,
	LOG_WARNING = 2,
	LOG_ERROR = 3,
	LOG_NONE = 4
};

// Global log level
extern LogLevel g_logLevel;

// Set the current log level
void setLogLevel(LogLevel level);

// Get log level from string (for command line parsing)
LogLevel getLogLevelFromString(const std::string& levelStr);

// Logging functions
void logDebug(const std::string& message);
void logInfo(const std::string& message);
void logWarning(const std::string& message);
void logError(const std::string& message);

// Convenience overloads for common types
void logDebug(const char* message);
void logInfo(const char* message);
void logWarning(const char* message);
void logError(const char* message);

// Helper to convert numbers to strings (C++98 compatible)
template<typename T>
std::string toString(T value) {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

#endif
