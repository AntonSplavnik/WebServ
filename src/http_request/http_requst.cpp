/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_requst.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:19 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/09/22 14:20:31 by antonsplavn      ###   ########.fr       */
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


/*
The Host header should be checked in the HTTP request parsing
  phase, specifically in the HttpRequest class after parsing
  headers.

  Where to Check Host Header:

  1. In HttpRequest::parseHeaders() method (http_requst.cpp:75-80)

  void HttpRequest::parseHeaders(){
      // Parse all headers into _headers map
      // Then validate required headers:

      // HTTP 1.1 requires Host header
      if (_headers.find("host") == _headers.end() &&
          _headers.find("Host") == _headers.end()) {
          _isValid = false;
          // Set error for 400 Bad Request
      }
  }

  2. Check in Server before method execution (server.cpp:186-194)

  HttpRequest requestParser;
  requestParser.parseRequest(_clients[fd]);

  // Check if request is valid (including Host header)
  if (!requestParser.isValid()) {
      // Send 400 Bad Request response
      _clients[fd].responseData = "HTTP/1.1 400 Bad
  Request\r\n...";
      return;
  }

  // Only execute methods if request is valid
  Methods method = stringToMethod(requestParser.getMethod());

  Best Practice Flow:

  1. Parse request → HttpRequest::parseRequest()
  2. Validate headers → Check Host header exists
  3. Set _isValid = false if missing Host header
  4. Server checks isValid() before executing GET/POST/DELETE
  5. Send 400 Bad Request if invalid
*/


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
