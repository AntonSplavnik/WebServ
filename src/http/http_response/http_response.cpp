/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_response.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/18 14:10:40 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/08 15:22:03 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "http_response.hpp"

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
		case 415: return "Unsupported Media Type";
		// Server Error
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Timeout";
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
	std::string errorPagePath = _customErrorPagePath;

	if (!errorPagePath.empty()) {
		std::ifstream errorFile(errorPagePath.c_str());
		if (errorFile.is_open()) {
			std::stringstream buffer;
			buffer << errorFile.rdbuf();
			_body = buffer.str();
			errorFile.close();
		}
	}
	if (_body.empty()) {
		_body = "<html><body><h1>Error " + std::to_string(_statusCode) + "</h1></body></html>";
	}
	_contentType = "text/html";
	_contentLength = _body.length();

	std::ostringstream oss;
	oss << _protocolVer << _statusCode << " " << _reasonPhrase << "\r\n"
		<< "Date: " << _date << "\r\n"
		<< "Server: " << _serverName << _serverVersion << "\r\n"
		<< "Content-Type: " << _contentType << "\r\n"
		<< "Content-Length: " << _contentLength << "\r\n"
		<< "Connection: close\r\n\r\n"
		<< _body;

	_response = oss.str();
}

void HttpResponse::generateNormalResponse() {

	_method = _request.getMethodEnum();
	_connectionType = _request.getConnectionType();

	if (_method == GET || _method == POST) {
		_body = extractBody();
		_contentType = getContentType();
		_contentLength = getContentLength();
	} else if (_method == DELETE) {
		_body = "";
		_contentType = "text/plain";
		_contentLength = _body.length();
	}

	std::ostringstream oss;

	oss << _protocolVer << _statusCode << " " << _reasonPhrase << "\r\n";
	oss << "Date: " << _date << "\r\n"
		<< "Server: " << _serverName << _serverVersion << "\r\n"
		<< "Content-Type: " << _contentType << "\r\n"
		<< "Content-Length: " << _contentLength << "\r\n"
		<< "Connection: " << _connectionType << "\r\n\r\n";

	if (_method == GET || _method == POST) {
		oss << _body;
	}
	_response = oss.str();
}

void HttpResponse::generateCgiResponse(const std::string& cgiOutput) {

	if (cgiOutput.find("Content-Type:") != std::string::npos) {
		_response = _protocolVer + std::to_string(_statusCode) + " " + _reasonPhrase + "\r\n" + cgiOutput;
	} else {
		_body = cgiOutput;
		_contentLength = _body.length();
		_contentType = "text/html";

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
}

void HttpResponse::generateResponse(int statusCode, bool isCgi, const std::string& cgiOutput) {
	_isCgiResponse = isCgi;
	_statusCode = statusCode;
	_reasonPhrase = getReasonPhrase();
	_date = getTimeNow();

	if (_statusCode >= 400) {
		generateErrorResponse();
		return;
	}
	generateCgiResponse(cgiOutput);
}

void HttpResponse::generateResponse(int statusCode) {
	_isCgiResponse = false;
	_statusCode = statusCode;
	_reasonPhrase = getReasonPhrase();
	_date = getTimeNow();

	if (_statusCode >= 400) {
		generateErrorResponse();
		return;
	}
	generateNormalResponse();
}

std::string HttpResponse::extractBody() {
	std::ifstream file(_filePath.c_str(), std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	return content;
}

void HttpResponse::setCustomErrorPage(const std::string& errorPagePath) {
    _customErrorPagePath = errorPagePath;
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
