/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_request.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:16:30 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/09/17 17:33:49 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST
#define HTTP_REQUEST

#include <iostream>
#include <string>
#include <map>
#include "server.hpp"

enum ParseState {PARSE_REQUEST_LINE, PARSE_HEADERS, PARSE_BODY};

class HttpRequest{

	public:

		HttpRequest();
		~HttpRequest();

		void parseRequest(ClientInfo& requestDate);

		void parseRequestLine();
		void parseBody();
		void parseHeaders();

		void parseMethod();
		void parsePath();
		void parseQuery();
		void parseVersion();

	private:

		std::string _method;
		std::string _path;
		std::string _version;
		// std::string query;
		std::map<std::string, std::string> _headers;
		std::string _body;
		bool _isValid;
};
#endif
