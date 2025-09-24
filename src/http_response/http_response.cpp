/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_response.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/18 14:10:40 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/09/24 17:49:30 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "http_response.hpp"


/*
	Here's a typical HTTP response structure:

  HTTP/1.1 200 OK
  Date: Tue, 24 Sep 2025 16:00:00 GMT
  Server: WebServ/1.0
  Content-Type: text/html
  Content-Length: 1234
  Connection: keep-alive

  <!DOCTYPE html>
  <html>
  <head>
      <title>Example Page</title>
  </head>
  <body>
      <h1>Hello World</h1>
      <p>This is the response body content.</p>
  </body>
  </html>

  Structure breakdown:

  1. Status Line: HTTP/1.1 200 OK
    - Protocol version
    - Status code (200)
    - Reason phrase (OK)
  2. Headers: Key-value pairs ending with \r\n
    - Date: When response was generated
    - Server: Server information
    - Content-Type: MIME type of content
    - Content-Length: Size of body in bytes
    - Connection: Connection handling
  3. Empty Line: \r\n separates headers from body
  4. Body: The actual content (HTML, JSON, file data, etc.)

  Common status codes:
  - 200 OK - Success
  - 404 Not Found - File not found
  - 500 Internal Server Error - Server error
  - 403 Forbidden - Access denied

  Each line ends with \r\n (carriage return + line feed).
*/
HttpResponse::HttpResponse(int statusCode)
	:_statusCode(statusCode), _serverName("WebServ"), _version(1.0f){}

HttpResponse::HttpResponse(int statusCode, std::string filePath)
	:_statusCode(statusCode), _filePath(filePath), _serverName("WebServ"), _version(1.0f){}

fileExtentions HttpResponse::getFileExtension(std::string filePath){
	//implementation here
	fileExtentions identifyedExtention;
	return identifyedExtention;
}

std::string HttpResponse::getContentType(){

	fileExtentions extention = getFileExtension(_filePath);

	switch (extention)
	{
	case HTML:
		return "text/html";
	case PDF:
		return "application/pdf";
	case JPEG:
		return "image/jpeg";
	case TXT:
		return "text/plaine";
	case PNG:
		return "image/png";
	}
}
std::string HttpResponse::getTimeNow(){
	time_t now = time(0);
	struct tm* gmtTime = gmtime(&now);
	char buffer[100];
	strftime(buffer, 100, "%a, %d %b %Y %H:%M:%S GMT", gmtTime);
	std::string httpTime = buffer;
	return httpTime;
}

char HttpResponse::getContentLength(){
	return (_contentLength = static_cast<char>(_body.length()));
}

void HttpResponse::generateResponse(){

	_reasonPhrase = // complete implementation
	_date = getTimeNow();
	_contentType = getContentType();
	_contentLength = getContentLength();
	_body = getBody();
	_contentLength = getContentLength();
	// Connection : keep-alive
}

void HttpResponse::setBody(std::string body){_body = body;}
void HttpResponse::setReasonPhrase(std::string reasonPhrase){_reasonPhrase = reasonPhrase;}
void HttpResponse::setVersion(std::string version){_version = version;}
void HttpResponse::setStatusCode(std::string statusCode){_statusCode = statusCode;}
void HttpResponse::setHeader(std::string headerKey, std::string headerValu){}


std::string HttpResponse::getBody(){
	std::ifstream file(_filePath, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	return content;
}
std::string HttpResponse::getPath(){}
std::string HttpResponse::getVersion(){}
std::string HttpResponse::getStatusCode(){}
std::string HttpResponse::getContentType(){}
