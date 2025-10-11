/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 14:23:52 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/11 17:38:08 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "src/server/server.hpp"
#include "src/config/config.hpp"
#include "src/exceptions/config_exceptions.hpp"
#include "server_controller.hpp"
#include <iostream>
#include <string>

int main(int argc, char *argv[]){


	try{
		if (argc != 2){
		std::cout << "Wrong config path" << std::endl;
		return 1;
		}

		std::cout << std::string(argv[1]) << std::endl;
		Config config;
		config.parseConfig(argv[1]);
		std::cout << "Config file loaded successfully" << std::endl;

		ServerController controller(config);
		controller.run();
	}
	catch(const std::exception& e){
		std::cerr << e.what() << '\n';
		return 1;
	}

	return 0;
}
