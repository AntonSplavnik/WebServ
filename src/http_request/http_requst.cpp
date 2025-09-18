/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_requst.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:19 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/09/18 14:09:17 by antonsplavn      ###   ########.fr       */
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

void HttpRequest::parseRequest(ClientInfo& requestDate){

	extractLineHeaderBodyLen();

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
}



void HttpRequest::extractLineHeaderBodyLen(){

	/*
	extraction logic
	_line =
	_heders =
	_body =
	_contentLength =
	*/
}

void HttpRequest::parseRequestLine(){

	/*
	exctraction logic
	_method =
	_path =
	_version =
	*/
}

void HttpRequest::parseHeaders(){

	/*
	_headers = builds header map
	*/
}

void HttpRequest::parseBody(){

	/*
	_body =
	*/
}

std::string HttpRequest::getMethod(){ return _method;}
