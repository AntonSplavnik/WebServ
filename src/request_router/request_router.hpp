/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request_router.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/21 17:44:00 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/26 11:19:07 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_ROUTER_HPP
#define REQUEST_ROUTER_HPP

#include <iostream>

#include "request.hpp"
#include "config.hpp"
class Connection;

enum RequestType{
	GET,
	DELETE,
	POST,
	CGI_POST,
	CGI_GET,
	REDIRECT
};

struct RoutingResult {
	bool success;
	int errorCode;
	ConfigData* serverConfig;
	const LocationConfig* location;
	std::string mappedPath;
	RequestType type;
	std::string cgiExtension;
};

class RequestRouter {

	public:
		RequestRouter(std::vector<ConfigData>& configs) :_configs(configs){}
		~RequestRouter(){}

		RoutingResult route(Connection& connection);

	private:
		std::vector<ConfigData>&	_configs;

		ConfigData& findServerConfig(const HttpRequest& request, int serverPort);	// Virtual hosting FindServerConfigByPort + MatchServerByHost
		bool validateBodySize(int contentLength, const LocationConfig*& location);
		bool validateMethod(const HttpRequest& request, const LocationConfig*& location);
		std::string mapPath(const HttpRequest& request, const LocationConfig*& matchedLocation);
		bool validatePathSecurity(const std::string& mappedPath, const std::string& allowedRoot);
		RequestType classify(const HttpRequest& req, const LocationConfig* location);
		std::string extractCgiExtension(const std::string& path, const LocationConfig* location);
};

#endif
