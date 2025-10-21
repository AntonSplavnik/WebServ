/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_response.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/18 14:10:40 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/15 14:11:56 by antonsplavn      ###   ########.fr       */
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

HttpResponse::HttpResponse(HttpRequest request, const ConfigData& configData)
	:_request(request), _method(), _protocolVer("HTTP/1.1 "),
	_serverName("WebServ"), _serverVersion(1.0f), _config(configData){}

HttpResponse::~HttpResponse(){}


fileExtentions HttpResponse::extractFileExtension(std::string filePath){

	size_t dotPos = filePath.find_last_of('.');
	if (dotPos == std::string::npos || dotPos == filePath.length() - 1) {
		return UNKNOWN;
	}
	std::string extension = filePath.substr(dotPos + 1);
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	if (extension == "html" || extension == "htm") return HTML;
	else if (extension == "pdf") return PDF;
	else if (extension == "jpg" || extension == "jpeg") return JPEG;
	else if (extension == "txt") return TXT;
	else if (extension == "png") return PNG;
	else if (extension == "js") return JS;

	else return UNKNOWN;
}

std::string HttpResponse::determineContentType() {

	fileExtentions extention = extractFileExtension(_filePath);

	switch (extention) {
		case JPEG: return "image/jpeg";
		case HTML: return "text/html";
		case TXT: return "text/plain";
		case JS: return "text/javascript";
		case PNG: return "image/png";
		case PDF: return "application/pdf";
		default: return "application/octet-stream";
	}
}

std::string HttpResponse::getReasonPhrase() {

	switch (_statusCode) {
		// Success
		case 200: return "OK";
		case 204: return "No Content";
		// Redirection
		case 301: return "Moved Permanently";
		case 304: return "Not Modified";
		// Client Error
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		// Server Error
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 503: return "Service Unavailable";
		default: return "Unknown";
	}
}

std::string HttpResponse::getTimeNow() {

	time_t now = time(0);
	struct tm* gmtTime = gmtime(&now);
	char buffer[100];
	strftime(buffer, 100, "%a, %d %b %Y %H:%M:%S GMT", gmtTime);
	std::string httpTime = buffer;
	return httpTime;
}

void HttpResponse::generateErrorResponse() {

	std::string errorPagePath = getErrorPagePath(_statusCode, _request.getPath());
    std::cout << "[DEBUG ERROR_PAGE] errorPagePath: " << errorPagePath << std::endl;

	if (!errorPagePath.empty()) {
		std::ifstream errorFile(errorPagePath.c_str());
		if (errorFile.is_open()) {
			std::stringstream buffer;
			buffer << errorFile.rdbuf();
			_contentType = "text/html";
			_body = buffer.str();
			_contentLength = _body.length();
		}
	}
	else {
		std::cout << "[DEBUG] Default page error" << std::endl;
		_contentType = "text/html";
		_body = "<html><body><h1>Error " + std::to_string(_statusCode) + "</h1></body></html>";
		_contentLength = _body.length();
	}

	std::ostringstream oss;
	oss << _protocolVer << _statusCode << " " << _reasonPhrase << "\r\n"
		<< "Date: " << getTimeNow() << "\r\n"
		<< "Server: " << _serverName << "/" << _serverVersion << "\r\n"
		<< "Content-Type: " << _contentType << "\r\n"
		<< "Content-Length: " << _contentLength << "\r\n"
		<< "Connection: " << _connectionType << "\r\n\r\n"
		<< _body;
	_response = oss.str();
}

void HttpResponse::generatePostResponse(){

	std::ostringstream oss;
	oss << _protocolVer << _statusCode << " " << _reasonPhrase << "\r\n"
		<< "Date: " << _date << "\r\n"
		<< "Server: " << _serverName << _serverVersion << "\r\n"
		<< "Content-Type: " << _contentType << "\r\n"
		<< "Content-Length: " << _contentLength << "\r\n"
		<< "Connection: " << _connectionType << "\r\n\r\n"
		<< _body;
	_response = oss.str();
}

void HttpResponse::generateGetResponse() {

	std::ostringstream oss;
	oss << _protocolVer << _statusCode << " " << _reasonPhrase << "\r\n"
		<< "Date: " << _date << "\r\n"
		<< "Server: " << _serverName << _serverVersion << "\r\n"
		<< "Content-Type: " << _contentType << "\r\n"
		<< "Content-Length: " << _contentLength << "\r\n"
		<< "Connection: " << _connectionType << "\r\n\r\n"
		<< _body;
	_response = oss.str();
}

void HttpResponse::generateDeleteResponse() {

	std::ostringstream oss;
	oss << _protocolVer << _statusCode << " " << _reasonPhrase << "\r\n"
		<< "Date: " << _date << "\r\n"
		<< "Server: " << _serverName << _serverVersion << "\r\n"
		<< "Content-Type: " << _contentType << "\r\n"
		<< "Content-Length: " << _contentLength << "\r\n"
		<< "Connection: " << _connectionType << "\r\n\r\n";
	_response = oss.str();
}

std::string HttpResponse::getErrorPagePath(int statusCode, const std::string& requestPath) const {
    const LocationConfig* location = _config.findMatchingLocation(requestPath);
    if (location && location->error_pages.count(statusCode)) {
        return _config.root + "/" + location->error_pages.find(statusCode)->second;
    }
    
    if (_config.error_pages.count(statusCode)) {
        return _config.root + "/" + _config.error_pages.find(statusCode)->second;
    }
    return "";
}

void HttpResponse::generateResponse(int statusCode) {

	_method = _request.getMethodEnum();
	_statusCode = statusCode;
	_reasonPhrase = getReasonPhrase(); //change to map
	_date = getTimeNow();

	if(_statusCode >= 400){
		generateErrorResponse();
		return;
	}

	_body = extractBody();
	_contentType = determineContentType();
	_contentLength = getContentLength();
	_connectionType = _request.getContenType();


	switch (_method)
	{
	case POST:
		generatePostResponse(); break;
	case GET:
		generateGetResponse(); break;
	case DELETE:
		generateDeleteResponse(); break;
	default:
		_statusCode = 405;
		_reasonPhrase = "Method Not Allowed";
		generateErrorResponse();
		break;
	}
}
std::string HttpResponse::extractBody() {
	std::ifstream file(_filePath.c_str(), std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	return content;
}


void HttpResponse::setBody(std::string body) {_body = body;}
void HttpResponse::setReasonPhrase(std::string reasonPhrase){_reasonPhrase = reasonPhrase;}
void HttpResponse::setVersion(float version) {_serverVersion = version;}
void HttpResponse::setStatusCode(int statusCode) {_statusCode = statusCode;}
void HttpResponse::setContentType(std::string contentType) { _contentType = contentType;}
void HttpResponse::setPath(std::string path) {_filePath = path;}

unsigned long HttpResponse::getContentLength() const {return _body.length();}
std::string HttpResponse::getBody() const {return _body;}
std::string HttpResponse::getPath()const {return _filePath;}
std::string HttpResponse::getContentType()const {return _contentType;}
float HttpResponse::getVersion() const {return _serverVersion;}
int HttpResponse::getStatusCode() const {return _statusCode;}
std::string HttpResponse::getResponse() const {return _response;}

/*
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

*/
