/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 14:23:52 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/09/16 17:20:00 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "src/server/server.hpp"

int main(){
	Server server;
	server.initialize();
	server.start();
	server.run();
	return 0;
}
