/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 14:23:52 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/07 15:07:59 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "src/event_loop/event_loop.hpp"
#include "src/config/config/config.hpp"
#include "src/config/config_exceptions/config_exceptions.hpp"
#include "src/logging/logger.hpp"
#include <iostream>
#include <string>
#include <csignal>

volatile sig_atomic_t g_shutdown = 0;

void signalHandler(int signum){
	(void)signum;

	g_shutdown = 1;
}

int main(int argc, char *argv[]){

	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	try{
		// Parse command line arguments
		std::string configPath;
		LogLevel logLevel = LOG_INFO; // Default

		if (argc == 2) {
			// Just config file
			configPath = argv[1];
		} else if (argc == 4) {
			// Config file + log level flag
			configPath = argv[1];
			std::string flag = argv[2];
			std::string level = argv[3];

			if (flag == "--log-level" || flag == "-l") {
				logLevel = getLogLevelFromString(level);
			} else {
				std::cerr << "Usage: " << argv[0] << " <config_file> [--log-level <debug|info|warning|error|none>]" << std::endl;
				return 1;
			}
		} else {
			std::cerr << "Usage: " << argv[0] << " <config_file> [--log-level <debug|info|warning|error|none>]" << std::endl;
			return 1;
		}

		// Set the log level
		setLogLevel(logLevel);

		logInfo("Starting WebServ with config: " + configPath);
		Config config;
		// parseConfig expects char*, need to cast away const (not modifying string in practice)
		config.parseConfig(const_cast<char*>(configPath.c_str()));
		logInfo("Config file loaded successfully");

		EventLoop controller(config);
		controller.run();
	}
	catch(const std::exception& e){
		logError(e.what());
		return 1;
	}

	return 0;
}
