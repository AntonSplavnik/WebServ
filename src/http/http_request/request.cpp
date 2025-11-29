/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:19 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/29 15:21:39 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "request.hpp"

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

void HttpRequest::parseRequestHeaders(const std::string requestData) {

	std::cout << "\n#######  HTTP PARSE REQUEST HEADERS #######" << std::endl;

	extractLineHeaderBodyLen(requestData);
	if (!_isValid) return;

	parseRequestLine();
	if (!_isValid) return;

	parseHeaders();
	if (!_isValid) return;

	std::cout << "#################################\n" << std::endl;
}
void HttpRequest::addBody(const std::string& requestBuffer){

	size_t bodyStart = requestBuffer.find("\r\n\r\n") + 4;
	_body = requestBuffer.substr(bodyStart);
	parseBody();
}

void HttpRequest::extractLineHeaderBodyLen(std::string rawData) {

	// Extract request line
	size_t firstCRLF = rawData.find("\r\n");
	if (firstCRLF == std::string::npos) {
		_isValid = false;
		return;
	}
	_requestLine = rawData.substr(0, firstCRLF);
	std::cout << "[DEBUG] _requestLine: " << _requestLine << std::endl;

	// Extract Header & Body
	size_t headerBodySeparator = rawData.find("\r\n\r\n");
	size_t headersStart = firstCRLF + 2; // After "GET /path HTTP/1.1\r\n"
	size_t headersLength = headerBodySeparator - headersStart;
	_rawHeaders = rawData.substr(headersStart, headersLength);
	_body = rawData.substr(headerBodySeparator + 4);

}

void HttpRequest::parseRequestLine() {

	if (_requestLine.empty()) {
		std::cout << " Error: Request line is empty" << std::endl;
		_isValid = false;
		return;
	}
	std::istringstream iss(_requestLine);

	iss >> _method;
	iss >> _path;
	iss >> _version;

	std::cout << "_method: " << _method << std::endl
			<< "_path: " << _path << std::endl
			<< "_version: " << _version << std::endl;

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

	size_t queryPos = _path.find('?');
	if (queryPos != std::string::npos) {
		_query = _path.substr(queryPos + 1);  // "q=test"
		_path = _path.substr(0, queryPos);   // "/cgi-bin/script.py"
	} else {
		_query = "";
	}
}
void HttpRequest::parseHeaders() {

	if (_rawHeaders.empty()) {
		std::cout << "No headers to parse" << std::endl;
		_isValid = false;
		return;
	}

	std::istringstream iss(_rawHeaders);
	std::string headerLine;
	int headerCount = 0;
	const int MAX_HEADERS = 100;

	while (std::getline(iss, headerLine))
	{
		// Remove \r if present
		if (!headerLine.empty() && headerLine[headerLine.length() - 1] == '\r')
			headerLine.erase(headerLine.length() - 1);
		// Skip empty line
		if (headerLine.empty()) continue;

		// Check header count limit
		if (++headerCount > MAX_HEADERS) {
			std::cout << "[ERROR] Too many headers (max " << MAX_HEADERS << ")" << std::endl;
			_isValid = false;
			return;
		}

		// Find ':'
		size_t pos = headerLine.find(':');
		if (pos == std::string::npos) {
			std::cout << "[ERROR] Invalid header line (no colon)" << std::endl;
			continue;
		}

		// Extract key and value
		std::string key = headerLine.substr(0, pos);
		std::string value = headerLine.substr(pos + 1);

		// Validate header name
		if (key.empty()) {
			std::cout << "[ERROR] Empty header name" << std::endl;
			continue;
		}

		// Check for spaces in header name (RFC violation)
		if (key.find(' ') != std::string::npos || key.find('\t') != std::string::npos) {
			std::cout << "[ERROR] Invalid header name (contains whitespace): " << key << std::endl;
			continue;
		}

		// Tolower key
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);

		// Trim leading/trailing whitespace from value
		size_t start = value.find_first_not_of(" \t");
		size_t end = value.find_last_not_of(" \t");
		if (start != std::string::npos)
			value = value.substr(start, end - start + 1);
		else
			value = "";

		_headers[key] = value;
	}

	// Only validate Host for HTTP/1.1
	if (_version == "HTTP/1.1") {
		std::map<std::string, std::string>::const_iterator hostIt = _headers.find("host");
		if (hostIt == _headers.end() || hostIt->second.empty()) {
			std::cout << "[ERROR] Missing Host header" << std::endl;
			_isValid = false;
			return;
		}
	}
}
void HttpRequest::parseBody() {

	if (_body.empty()) {
		_isValid = false;
		return;
	}

	// Validate Content-Length if present
	if (getContentLength() != _body.length()) {
		_isValid = false;
		std::cout << "Content-Length mismatch"
				  << getContentLength()
				  << ", got " << _body.length()
				  << std::endl;
	}
}

std::string HttpRequest::getHeaderValue(const std::string& key, const std::string& defaultValue) const {
	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	if (it != _headers.end())
		return it->second;
	else
		return defaultValue;
}
std::string HttpRequest::getContentType() const {
	return getHeaderValue("content-type");
}
std::string HttpRequest::getConnectionType() const {
	// If client used HTTP/1.1 → default keep-alive
	if (_version == "HTTP/1.1") {
		return getHeaderValue("connection", "keep-alive");
	}
	// If client used HTTP/1.0 → default close
	else {
		return getHeaderValue("connection", "close");
	}
}
unsigned long HttpRequest::getContentLength() const {
	std::string value = getHeaderValue("content-length", "0");
	if (value.empty())
		return 0;
	return std::strtoul(value.c_str(), NULL, 10);
}
std::string HttpRequest::getTransferEncoding() const {
	return getHeaderValue("transfer-encoding");
}
std::string HttpRequest::getHost() const {
	return getHeaderValue("host");
}
std::vector<std::string> HttpRequest::getCgiHeadersString() const {
	std::vector<std::string> cgiHeaders;
	std::map<std::string, std::string>::const_iterator it = _headers.begin();
	for (; it != _headers.end(); ++it)
	{
    	std::string key = "HTTP_" + it->first;
    	std::transform(key.begin(), key.end(), key.begin(), ::toupper);
    	std::replace(key.begin(), key.end(), '-', '_');
		std::string buffer = key + "=" + it->second;
		cgiHeaders.push_back(buffer);
	}
	return cgiHeaders;
}
std::string HttpRequest::getUri() const {
	if (_query.empty())
		return _path;
	return  _path + "?" + _query;

}
