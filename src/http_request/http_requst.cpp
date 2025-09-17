/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_requst.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:19 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/09/17 17:37:19 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "http_request.hpp"

// std::string method;
// std::string path;
// std::string query;
// std::string version;
// std::map<std::string, std::string> headers;
// std::string body;
// bool isValid;



HttpRequest::HttpRequest()
	: _method(), _path(), _version(), _headers(), _body(), _isValid(false){

}

HttpRequest::~HttpRequest(){}

typedef struct s_dataToParse{
	std::string line;
	std::string headers;
	std::string body;
}t_dataToParse;

void HttpRequest::parseRequest(ClientInfo& requestDate){

	t_dataToParse data;

	splitRequest(requestDate.requstData, data&);

	ParseState state = PARSE_REQUEST_LINE;

	switch (state) {
	case PARSE_REQUEST_LINE:
		parseRequestLine();
		state = PARSE_HEADERS;
		break;
	case PARSE_HEADERS:
		parseHeaders();
		state = PARSE_BODY;
		break;
	case PARSE_BODY:
		parseBody();
		break;
	}

	// if (parseline())

	// 	ifparseMethod();



	// switch():
	// 	case(GET)
	// 		handleGET
	// 	case(POST)
	// 		handlePOST
	// 	case(DELETE)
	// 		handleDELETE
}

void parseRequestLine(t_dataToParse data){

	data.
}
