/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_request.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:16:30 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/09/25 15:27:18 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST
#define HTTP_REQUEST

#include <iostream>
#include <string>
#include <map>
#include "server.hpp"

enum ParseState {PARSE_REQUEST_LINE, PARSE_HEADERS, PARSE_BODY, PARSE_COMPLETE};

class HttpRequest{

	public:
		HttpRequest();
		~HttpRequest();

		void parseRequest(ClientInfo& requestDate);
		
		void extractLineHeaderBodyLen(const std::string& rawData);
		void parseRequestLine();
		void parseBody();
		void parseHeaders();

		void parseMethod();
		void parsePath();
		void parseQuery();
		void parseVersion();

		std::string getMethod() const;
		std::string getPath() const;
		std::string getVersion() const;
		std::string getContenType() const;
		std::string getHeader(const std::string& headerName) const;
		bool isValid() const;

	private:
		std::string _line;
		std::string _rawHeaders;  // Headers bruts (string)
		std::string _body;
		long long 		_contentLength;

		//reqest line
		std::string _method;
		std::string _path;
		std::string _version;

		//headers
		// std::string query;
		std::map<std::string, std::string> _headers;  // Headers parsés (map)
		bool _isValid;
};
#endif
