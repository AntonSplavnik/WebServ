/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 14:23:52 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/03 15:25:45 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "src/server/server.hpp"
#include "src/config/config.hpp"
#include "src/exceptions/config_exceptions.hpp"
#include <iostream>

int main(int argc, char *argv[]){
	if (argc != 2)
	{
		std::cout << "Wrong config path" << std::endl;
		return 1;
	}
	Config config;
	try
	{
		config.parseConfig(argv);
	}
	catch (const ConfigParseException &e)
	{
		std::cerr << "Error parsing config file: " << e.what() << std::endl;
		return 1;
	}
	std::cout << "Config file loaded successfully" << std::endl;
	try
	{
		Server server;
		server.initialize();
		server.start();
		server.run();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}

	return 0;
}
