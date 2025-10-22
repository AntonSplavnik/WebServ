/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_response.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/18 14:10:40 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/21 17:48:40 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "http_response.hpp"

HttpResponse::HttpResponse(HttpRequest request, const ConfigData& configData)
	:_request(request), _method(), _protocolVer("HTTP/1.1 "),
	_serverName("WebServ"), _serverVersion(1.0f), _config(configData){}

HttpResponse::~HttpResponse(){}

const std::map<std::string, std::string> HttpResponse::_mimeTypes = HttpResponse::initMimeTypes();

std::map<std::string, std::string> HttpResponse::initMimeTypes() {
    std::map<std::string, std::string> types;
    
	// Texte
	types["html"] = "text/html; charset=utf-8";
	types["htm"] = "text/html; charset=utf-8";
	types["css"] = "text/css; charset=utf-8";
	types["js"] = "text/javascript; charset=utf-8";
	types["json"] = "application/json; charset=utf-8";
	types["xml"] = "application/xml; charset=utf-8";
	types["txt"] = "text/plain; charset=utf-8";
	types["csv"] = "text/csv; charset=utf-8";
	types["md"] = "text/markdown; charset=utf-8";

	// Images
    types["jpg"] = "image/jpeg";
    types["jpeg"] = "image/jpeg";
    types["png"] = "image/png";
    types["gif"] = "image/gif";
    types["svg"] = "image/svg+xml";
    types["webp"] = "image/webp";
	types["ico"] = "image/x-icon";
	types["bmp"] = "image/bmp";

	// Audio
	types["mp3"] = "audio/mpeg";
	types["wav"] = "audio/wav";
	types["ogg"] = "audio/ogg";
	types["m4a"] = "audio/mp4";

	// Vidéo
	types["mp4"] = "video/mp4";
	types["webm"] = "video/webm";
	types["avi"] = "video/x-msvideo";
	types["mov"] = "video/quicktime";

	// Application
	types["pdf"] = "application/pdf";
	types["zip"] = "application/zip";
	types["tar"] = "application/x-tar";
	types["gz"] = "application/gzip";
	types["wasm"] = "application/wasm";

	// Polices
	types["ttf"] = "font/ttf";
	types["woff"] = "font/woff";
	types["woff2"] = "font/woff2";
	
	return types;
}

// MIME = Multipurpose Internet Mail Extensions
// Return MIME from map _mimeTypes
std::string HttpResponse::getMimeType(const std::string& extension) const {

	std::map<std::string, std::string>::const_iterator it = _mimeTypes.find(extension);
    
	if (it != _mimeTypes.end()) {
		return it->second;
	}
    return "application/octet-stream";
}

//function to exctract extension after '.' and lowercase it
std::string HttpResponse::extractFileExtension(std::string filePath){

	//Find extension after '.'
	size_t dotPos = filePath.find_last_of('.');
	if (dotPos == std::string::npos || dotPos == filePath.length() - 1) {
		return "";
    }
	std::string extension = filePath.substr(dotPos + 1);
    
    // Convert to lowercase
	for (size_t i = 0; i < extension.length(); ++i) {
		extension[i] = std::tolower(extension[i]);
	}
	return extension;
}

//example : Swith index.html to "text/html; charset=utf-8"
std::string HttpResponse::determineContentType() {

	std::string extension = extractFileExtension(_filePath);
	return getMimeType(extension);
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

void HttpResponse::generateGpResponse(){

	std::ostringstream oss;
	oss << _protocolVer << _statusCode << " " << _reasonPhrase << "\r\n"
		<< "Server: " << _serverName << "/" << _serverVersion << "\r\n"
		<< "Date: " << _date << "\r\n"
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
		<< "Server: " << _serverName << "/" << _serverVersion << "\r\n"
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
	_reasonPhrase = getReasonPhrase();
	_date = getTimeNow();

	if(_statusCode >= 400){
		generateErrorResponse();
		return;
	}

	_body = extractBody();
	_contentType = determineContentType();
	_contentLength = getContentLength();
	_connectionType = _request.getConnectionType();


	switch (_method)
	{
	case POST:
	case GET:
		generateGpResponse(); break;
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
