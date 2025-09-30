/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_request.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:19 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/09/25 15:26:51 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "http_request.hpp"
#include <sstream>
#include <cctype>

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

void HttpRequest::parseRequest(ClientInfo& requestData){

/*	
Critical HTTP parsing test scenarios
	- Invalid request line structure
	- Missing mandatory headers (Host for HTTP/1.1)
	- Payload size exceeding configured limits
	- URI with forbidden/encoded characters
	- Query string with multiple parameters
	- Content-Length header validation failures
	- Non-implemented HTTP method requests
*/
	extractLineHeaderBodyLen(requestData.requstData);

	ParseState state = PARSE_REQUEST_LINE;
	while(state != PARSE_COMPLETE && _isValid) {
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
			state = PARSE_COMPLETE;
			break;
		}
	}
}

void HttpRequest::extractLineHeaderBodyLen(const std::string& rawData){
	
	size_t firstCRLF = rawData.find("\r\n");
	if (firstCRLF == std::string::npos) {
		_isValid = false;
		return;
	}
	_line = rawData.substr(0, firstCRLF);
	
	size_t headerBodySeparator = rawData.find("\r\n\r\n");
	if (headerBodySeparator == std::string::npos) {
		// Pas de body (GET request typique)
		_rawHeaders = rawData.substr(firstCRLF + 2); // After request line
		_body = "";
		_contentLength = 0;
	} else {
		size_t headersStart = firstCRLF + 2; // After "GET /path HTTP/1.1\r\n"
		size_t headersLength = headerBodySeparator - headersStart;
		_rawHeaders = rawData.substr(headersStart, headersLength);
		// ÉTAPE 4: Extract BODY (after \r\n\r\n)
		_body = rawData.substr(headerBodySeparator + 4);
		_contentLength = _body.length();
	}
	
	std::cout << "  HTTP Parsing Results:" << std::endl;
	std::cout << "  Request Line: [" << _line << "]" << std::endl;
	std::cout << "  Raw Headers Length: " << _rawHeaders.length() << " bytes" << std::endl;
	std::cout << "  Body Length: " << _body.length() << " bytes" << std::endl;
	std::cout << "  Content-Length: " << _contentLength << std::endl;
}

void HttpRequest::parseRequestLine(){

	if (_line.empty()) {
		std::cout << " Error: Request line is empty" << std::endl;
		_isValid = false;
		return;
	}
	
	std::istringstream iss(_line);
	
	iss >> _method;
	iss >> _path;
	iss >> _version;

	if (_method.empty() || _path.empty() || _version.empty()) {
		std::cout << " Error: Invalid request line format" << std::endl;
		_isValid = false;
		return;
	}
	
	if (_method != "GET" && _method != "POST" && _method != "DELETE") {
		std::cout << " Error: Unknown HTTP method: " << _method << std::endl;
		_isValid = false;
		return;
	}
	
	if (_version != "HTTP/1.1" && _version != "HTTP/1.0") {
		std::cout << "Error : Unsupported HTTP version: " << _version << std::endl;
		_isValid = false;
		return;
	}
	
	// Debug output parsing line
	std::cout << "   Request Line Parsed:" << std::endl;
	std::cout << "   METHOD: [" << _method << "]" << std::endl;
	std::cout << "   PATH: [" << _path << "]" << std::endl;
	std::cout << "   VERSION: [" << _version << "]" << std::endl;
}

void HttpRequest::parseHeaders(){
	
	if (_rawHeaders.empty()) {
		std::cout << "No headers to parse" << std::endl;
		_isValid = false;
		return;
	}
	
	std::istringstream iss(_rawHeaders);
	std::string headerLine;

	while (std::getline(iss, headerLine))
	{
		// 1) Skip empty line
		// 2) Delete \r at the end
		// 3) Find : ':'
		// 4) extract key and value
		// 5) convert in lowercase
		// 6) save into map
	}
		
}

void HttpRequest::parseBody(){

	/*
	_body =
	*/
}

std::string HttpRequest::getMethod() const { return _method;}
std::string HttpRequest::getPath() const {return _path;}
std::string HttpRequest::getVersion() const {return _version;}
std::string HttpRequest::getContenType() const {
	std::map<std::string, std::string>::const_iterator it = _headers.find("content-type");
	if (it != _headers.end())
		return it->second;
	else
		return "";  // Pas de Content-Type par défaut
}

std::string HttpRequest::getHeader(const std::string& headerName) const {
	// Convertir le nom en minuscules pour la recherche
	std::string lowerHeaderName = headerName;
	for (size_t i = 0; i < lowerHeaderName.length(); ++i) {
		lowerHeaderName[i] = std::tolower(lowerHeaderName[i]);
	}
	
	std::map<std::string, std::string>::const_iterator it = _headers.find(lowerHeaderName);
	if (it != _headers.end())
		return it->second;
	else
		return "";  // Header non trouvé
}

bool HttpRequest::isValid() const {
	return _isValid;
}

