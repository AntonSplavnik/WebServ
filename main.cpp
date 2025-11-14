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

		EventLoop controller(config);
		controller.run();
	}
	catch(const std::exception& e){
		std::cerr << e.what() << '\n';
		return 1;
	}

	return 0;
}
