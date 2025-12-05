/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/05 00:00:00 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/12/05 00:00:00 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "logger.hpp"
#include <iostream>
#include <algorithm>

// Global log level - defaults to INFO
LogLevel g_logLevel = LOG_INFO;

void setLogLevel(LogLevel level) {
	g_logLevel = level;
}

LogLevel getLogLevelFromString(const std::string& levelStr) {
	std::string lower = levelStr;

	// Convert to lowercase (C++98 compatible)
	for (size_t i = 0; i < lower.length(); ++i) {
		lower[i] = std::tolower(lower[i]);
	}

	if (lower == "debug") return LOG_DEBUG;
	if (lower == "info") return LOG_INFO;
	if (lower == "warning") return LOG_WARNING;
	if (lower == "error") return LOG_ERROR;
	if (lower == "none") return LOG_NONE;

	return LOG_INFO; // Default
}

void logDebug(const std::string& message) {
	if (g_logLevel <= LOG_DEBUG) {
		std::cout << "[DEBUG] " << message << std::endl;
	}
}

void logInfo(const std::string& message) {
	if (g_logLevel <= LOG_INFO) {
		std::cout << "[INFO] " << message << std::endl;
	}
}

void logWarning(const std::string& message) {
	if (g_logLevel <= LOG_WARNING) {
		std::cerr << "[WARNING] " << message << std::endl;
	}
}

void logError(const std::string& message) {
	if (g_logLevel <= LOG_ERROR) {
		std::cerr << "[ERROR] " << message << std::endl;
	}
}

// C-string overloads
void logDebug(const char* message) {
	logDebug(std::string(message));
}

void logInfo(const char* message) {
	logInfo(std::string(message));
}

void logWarning(const char* message) {
	logWarning(std::string(message));
}

void logError(const char* message) {
	logError(std::string(message));
}
