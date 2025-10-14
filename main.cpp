/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 14:23:52 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/14 16:03:29 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "src/server/server.hpp"
#include "src/config/config.hpp"
#include "src/exceptions/config_exceptions.hpp"
#include "server_controller.hpp"
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
		if (argc != 2){
		std::cout << "Wrong config path" << std::endl;
		return 1;
		}

		std::cout << std::string(argv[1]) << std::endl;
		Config config;
		config.parseConfig(argv[1]);
		std::cout << "Config file loaded successfully" << std::endl;

		std::cout << config.getServers()[0].upload_store << std::endl;

		ServerController controller(config);
		controller.run();
	}
	catch(const std::exception& e){
		std::cerr << e.what() << '\n';
		return 1;
	}

	return 0;
}
