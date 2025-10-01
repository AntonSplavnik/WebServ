/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_request.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:19 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/01 13:34:41 by antonsplavn      ###   ########.fr       */
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
	: _requestLine(), _body(), _contentLength(), _method(), _path(), _version(), _headers(), _isValid(false){
}

HttpRequest::~HttpRequest(){}

void HttpRequest::parseRequest(const std::string requestData){

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
	extractLineHeaderBodyLen(requestData);

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

void HttpRequest::extractLineHeaderBodyLen(std::string rawData) {
	
	//STEP 1 -> Extract request line
	size_t firstCRLF = rawData.find("\r\n");
	if (firstCRLF == std::string::npos) {
		_isValid = false;
		return;
	}
	_requestLine = rawData.substr(0, firstCRLF);
	
	//STEP 2 -> extract Header & Body 
	size_t headerBodySeparator = rawData.find("\r\n\r\n");
	if (headerBodySeparator == std::string::npos) {
		_rawHeaders = rawData.substr(firstCRLF + 2); // After request line
		_body = "";
		_contentLength = 0;
	} else {
		size_t headersStart = firstCRLF + 2; // After "GET /path HTTP/1.1\r\n"
		size_t headersLength = headerBodySeparator - headersStart;
		_rawHeaders = rawData.substr(headersStart, headersLength);
		_body = rawData.substr(headerBodySeparator + 4);
		_contentLength = _body.length();
	}
}

void HttpRequest::parseRequestLine(){

	if (_requestLine.empty()) {
		std::cout << " Error: Request line is empty" << std::endl;
		_isValid = false;
		return;
	}
	
	std::istringstream iss(_requestLine);
	
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
}

void HttpRequest::parseHeaders(){
	
/*
	Todo: 
	- Preserves whitespace in header values 
	- Normalized key to lowercase OK 
	- No input validation
*/

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
		if (headerLine.empty()) continue;
		// 2) Find : ':'
		size_t pos = headerLine.find(':');
		// 3) extract key and value
        if (pos != std::string::npos) {
            std::string key = headerLine.substr(0, pos);
            std::string value = headerLine.substr(pos + 1);
		// 4) tolower key
			std::transform(key.begin(), key.end(), key.begin(), ::tolower);

            _headers[key] = value;
		}
	}
}

void HttpRequest::parseBody(){

    if (_body.empty()) {
        std::cout << "No body to parse" << std::endl;
        return;
    }
}

void HttpRequest::setMethod(std::string method) {_method = method;}
void HttpRequest::setPath(std::string path){_path = path;}
void HttpRequest::setVersion(std::string version){_version = version;}
// void HttpRequest::setContentType(std::string ContentType){}
void HttpRequest::setContentLength(unsigned long contentLength){_contentLength = contentLength;}


std::string HttpRequest::getRequstLine() const {return _requestLine;}
std::string HttpRequest::getBody() const {return _body;}
std::string HttpRequest::getRawHeaders() const {return _rawHeaders;}
unsigned long HttpRequest::getContentLength() const {return _contentLength;}

void HttpRequest::setRequstLine(std::string requestLine) {_requestLine = requestLine;}
void HttpRequest::setBody(std::string body) {_body = body;}
void HttpRequest::setRawHeaders(std::string rawHeaders) {_rawHeaders = rawHeaders;}

std::string HttpRequest::getMethod() const { return _method;}
std::string HttpRequest::getPath() const {return _path;}
std::string HttpRequest::getVersion() const {return _version;}

std::string HttpRequest::getContenType() const {
	std::map<std::string, std::string>::const_iterator it = _headers.find("Connection");
	if (it != _headers.end())
		return it->second;
	else
		return "Keep-alive";
}
