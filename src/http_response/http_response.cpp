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

HttpResponse::HttpResponse(HttpRequest request)
	:_request(request), _method(), _protocolVer("HTTP/1.1 "),
	_serverName("WebServ"), _serverVersion(1.0f){}

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

std::string HttpResponse::getContentType() {

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

void HttpResponse::generateErrorResponse() //TODO: load a custom HTML file per error code (like error_pages/404.html, error_pages/500.html), and fall back to this default if not found
    //TODO: check if it works with sattic
{

    std::ostringstream body;

    // Simple HTML body for the error
    body << "<html><head><title>" << _statusCode << " " << _reasonPhrase
         << "</title></head><body><center><h1>" << _statusCode << " "
         << _reasonPhrase
         << "</h1></center><hr><center>WebServ42/1.0</center></body></html>";

    _body = body.str();
    _contentType = "text/html";
    _contentLength = _body.size();
    _connectionType = "close";  // close connection on error

    // Build full HTTP response
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


void HttpResponse::generateCgiResponse(const std::string &cgiOutput)
{
    // --- 1️⃣ Find headers/body separator ---
    size_t headerEnd = cgiOutput.find("\r\n\r\n");
    size_t separatorLength = 4;
    if (headerEnd == std::string::npos) {
        headerEnd = cgiOutput.find("\n\n");
        separatorLength = 2;
    }

    std::string headers = cgiOutput.substr(0, headerEnd);
    std::string body = cgiOutput.substr(headerEnd + separatorLength);

    // --- 2️⃣ Default values ---
    int statusCode = 200;
	std::string statusText = "OK";
    std::string contentType = "text/plain";
    std::string connection = "keep-alive";
    long contentLength = -1; // means not provided by CGI

    // --- 3️⃣ Parse CGI headers manually ---
    std::istringstream hdrStream(headers);
    std::string line;
    while (std::getline(hdrStream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (line.find("Content-Type:") == 0)
        {
            size_t start = line.find_first_not_of(" \t", 13);
            if (start != std::string::npos)
                contentType = line.substr(start);
        }
        else if (line.find("Content-Length:") == 0)
        {
            size_t start = line.find_first_not_of(" \t", 15);
            if (start != std::string::npos)
            {
                std::string lenStr = line.substr(start);
                std::istringstream iss(lenStr);
                iss >> contentLength; // simple C++98 safe parsing
                if (iss.fail())
                    contentLength = -1;
            }
        }
        else if (line.find("Connection:") == 0)
        {
            size_t start = line.find_first_not_of(" \t", 11);
            if (start != std::string::npos)
                connection = line.substr(start);
        }
        else if (line.find("Status:") == 0) {
    		size_t start = line.find_first_not_of(" \t", 7);
    		if (start != std::string::npos) {
        		std::istringstream iss(line.substr(start));
        		iss >> statusCode;
        		std::getline(iss, statusText);
        		if (!statusText.empty() && statusText[0] == ' ')
            		statusText.erase(0, 1);
    		}
		}
    }

    // ---  Fallback if Content-Length missing ---
    if (contentLength < 0)
        contentLength = static_cast<long>(body.size());

    // ---  Build final HTTP response ---
    std::ostringstream oss;
    oss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n"
        << "Date: " << getTimeNow() << "\r\n"
        << "Server: WebServ/1.0\r\n"
        << "Content-Type: " << contentType << "\r\n"
        << "Content-Length: " << contentLength << "\r\n"
        << "Connection: " << connection << "\r\n\r\n"
        << body;

    _response = oss.str();
}


void HttpResponse::generateResponse(int statusCode, bool isCgi, const std::string& cgiOutput)
{
	_method = _request.getMethodEnum();
	_statusCode = statusCode;
	_reasonPhrase = getReasonPhrase();
	_date = getTimeNow();

	//  --- 1. Handle CGI case ---
	if (isCgi)
	{
		generateCgiResponse(cgiOutput);
		return;
	}

	// --- 2. Handle HTTP errors ---
	if (_statusCode >= 400)
	{
		generateErrorResponse();
		return;
	}

	//  --- 3. Normal file-based response ---
	_body = extractBody();
	_contentType = getContentType();
	_contentLength = getContentLength();

	// Determine connection type: default keep-alive
	std::map<std::string, std::string>::const_iterator it = _request.getHeaders().find("connection");
	if (it != _request.getHeaders().end())
		_connectionType = it->second;
	else
		_connectionType = "keep-alive";

	//  --- 4. Generate based on HTTP method ---
	switch (_method)
	{
		case POST:
			generatePostResponse();
			break;
		case GET:
			generateGetResponse();
			break;
		case DELETE:
			generateDeleteResponse();
			break;
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
void HttpResponse::setPath(std::string path) {_filePath = path;}


unsigned long HttpResponse::getContentLength() const {return _body.length();}
std::string HttpResponse::getBody() const {return _body;}
std::string HttpResponse::getPath()const {return _filePath;}
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
