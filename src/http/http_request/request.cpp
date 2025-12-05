/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:18:19 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/12/02 22:49:50 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "request.hpp"
#include "logger.hpp"

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

void HttpRequest::setFallbackValues() {
	_method = "GET";
	_path = "/";
	_version = "HTTP/1.1";
	_query = "";
	_headers.clear();
}

void HttpRequest::parseRequestHeaders(const std::string requestData) {

	logDebug("\n#######  HTTP PARSER #######");

	extractLineHeaderBodyLen(requestData);
	if (_statusCode != 0) return;

	parseRequestLine();
	if (_statusCode != 0) return;

	parseHeaders();
	if (_statusCode != 0) return;

	logDebug("#################################\n");
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
		logError("Missing \\r\\n after request line");
		_statusCode = 400;
		return;
	}
	_requestLine = rawData.substr(0, firstCRLF);
	logDebug("_requestLine: " + _requestLine);

	// Extract Header & Body
	size_t headerBodySeparator = rawData.find("\r\n\r\n");
	if (headerBodySeparator == std::string::npos) {
		logError("Missing \\r\\n\\r\\n separator between headers and body");
		_statusCode = 400;
		return;
	}
	size_t headersStart = firstCRLF + 2; // After "GET /path HTTP/1.1\r\n"
	size_t headersLength = headerBodySeparator - headersStart;
	_rawHeaders = rawData.substr(headersStart, headersLength);
	_body = rawData.substr(headerBodySeparator + 4);

}

void HttpRequest::parseRequestLine() {

	if (_requestLine.empty()) {
		logError("Request line is empty");
		setFallbackValues();
		_statusCode = 400;
		return;
	}
	std::istringstream iss(_requestLine);

	iss >> _method;
	iss >> _path;
	iss >> _version;

	logDebug("_method: " + _method);
	logDebug("_path: " + _path);
	logDebug("_version: " + _version);

	// Check for unknown/empty HTTP method first (501)
	if (_method.empty() || (_method != "GET" && _method != "POST" && _method != "DELETE")) {
		logError("Unknown or invalid HTTP method: " + _method);
		setFallbackValues();
		_statusCode = 501;
		return;
	}

	// Check for unsupported HTTP version (505)
	if (_version.empty() || (_version != "HTTP/1.1" && _version != "HTTP/1.0")) {
		logError("Unsupported HTTP version: " + _version);
		setFallbackValues();
		_statusCode = 505;
		return;
	}

	// Check path and stringstream state (400)
	if (!iss || _path.empty()) {
		logError("Invalid request line format");
		setFallbackValues();
		_statusCode = 400;
		return;
	}

	// Extract query string
	size_t queryPos = _path.find('?');
	if (queryPos != std::string::npos) {
		_query = _path.substr(queryPos + 1);  // "q=test"
		_path = _path.substr(0, queryPos);   // "/cgi-bin/script.py"

		// Validate query string format
		if (!_query.empty()) {
			// Check for invalid starting characters
			if (_query[0] == '&' || _query[0] == '=' || _query[0] == '?') {
				logError("Invalid query string start");
				setFallbackValues();
				_statusCode = 400;
				return;
			}

			// Check for invalid patterns
			if (_query.find("==") != std::string::npos ||
				_query.find("&&") != std::string::npos ||
				_query.find("??") != std::string::npos) {
				logError("Invalid query string (double separator)");
				setFallbackValues();
				_statusCode = 400;
				return;
			}

			// Check for trailing invalid characters
			size_t len = _query.length();
			if (_query[len - 1] == '=' || _query[len - 1] == '&') {
				logError("Invalid query string (trailing separator)");
				setFallbackValues();
				_statusCode = 400;
				return;
			}

			// Validate each parameter (key=value format)
			size_t pos = 0;
			while (pos < _query.length()) {
				size_t ampPos = _query.find('&', pos);
				if (ampPos == std::string::npos) {
					ampPos = _query.length();
				}

				std::string param = _query.substr(pos, ampPos - pos);
				size_t eqPos = param.find('=');

				// Each parameter must have exactly one '=' with key and value
				if (eqPos == std::string::npos || eqPos == 0 || eqPos == param.length() - 1) {
					logError("Invalid query parameter format");
					setFallbackValues();
					_statusCode = 400;
					return;
				}

				pos = ampPos + 1;
			}
		}
	} else {
		_query = "";
	}
}
void HttpRequest::parseHeaders() {

	if (_rawHeaders.empty()) {
		logDebug("No headers to parse");
		_statusCode = 400;
		return;
	}

	// Validate proper CRLF line endings - check for bare \n
	for (size_t i = 0; i < _rawHeaders.length(); ++i) {
		if (_rawHeaders[i] == '\n') {
			// Check if \n is preceded by \r
			if (i == 0 || _rawHeaders[i - 1] != '\r') {
				logError("Invalid line ending (bare \\n without \\r)");
				_statusCode = 400;
				return;
			}
		}
	}

	std::istringstream iss(_rawHeaders);
	std::string headerLine;
	int headerCount = 0;
	const int MAX_HEADERS = 100;

	while (std::getline(iss, headerLine))
	{
		// Remove \r if present (handles both \r\n and \n line endings)
		if (!headerLine.empty() && headerLine[headerLine.length() - 1] == '\r')
			headerLine.erase(headerLine.length() - 1);

		// Skip empty line
		if (headerLine.empty()) continue;

		// Check header count limit
		if (++headerCount > MAX_HEADERS) {
			logError("Too many headers (max " + toString(MAX_HEADERS) + ")");
			_statusCode = 431;
			return;
		}

		// Find ':'
		size_t pos = headerLine.find(':');
		if (pos == std::string::npos) {
			logError("Invalid header line (no colon)");
			_statusCode = 400;
			return;
		}

		// Check for double colon (Host::)
		if (pos + 1 < headerLine.length() && headerLine[pos + 1] == ':') {
			logError("Invalid header format (double colon)");
			_statusCode = 400;
			return;
		}

		// Extract key and value
		std::string key = headerLine.substr(0, pos);
		std::string value = headerLine.substr(pos + 1);

		// Validate header name
		if (key.empty()) {
			logError("Empty header name");
			_statusCode = 400;
			return;
		}

		// Check for spaces in header name (RFC violation)
		if (key.find(' ') != std::string::npos || key.find('\t') != std::string::npos) {
			logError("Invalid header name (contains whitespace): " + key);
			_statusCode = 400;
			return;
		}

		// RFC 7230: requires at least one space/tab after colon
		if (value.empty() || (value[0] != ' ' && value[0] != '\t')) {
			logError("Missing space after colon in header");
			_statusCode = 400;
			return;
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
			logError("Missing Host header");
			_statusCode = 400;
			return;
		}
	}

	// Validate POST request headers
	if (_method == "POST") {
		// Check for Content-Length header (411 if missing)
		std::map<std::string, std::string>::const_iterator clIt = _headers.find("content-length");
		if (clIt == _headers.end() || clIt->second.empty()) {
			logError("POST request missing Content-Length header");
			_statusCode = 411;
			return;
		}

		// Validate Content-Length is a valid number
		const std::string& clValue = clIt->second;
		for (size_t i = 0; i < clValue.length(); ++i) {
			if (!std::isdigit(clValue[i])) {
				logError("Invalid Content-Length value");
				_statusCode = 400;
				return;
			}
		}

		// Check for Content-Type header (400 if missing)
		std::map<std::string, std::string>::const_iterator ctIt = _headers.find("content-type");
		if (ctIt == _headers.end() || ctIt->second.empty()) {
			logError("POST request missing Content-Type header");
			_statusCode = 400;
			return;
		}
	}

	std::map<std::string, std::string>::const_iterator hostIt = _headers.begin();
	for(; hostIt != _headers.end(); ++hostIt) {
		logDebug(hostIt->first + " : " + hostIt->second);
	}
}
void HttpRequest::parseBody() {

	if (_body.empty()) {
		// Empty body is valid if Content-Length is 0 or absent
		if (getContentLength() == 0) {
			return;
		}
		logError("Expected body but received none");
		_statusCode = 400;
		return;
	}

	// Validate Content-Length if present
	if (getContentLength() != _body.length()) {
		logError("Content-Length mismatch: expected " + toString(getContentLength()) + ", got " + toString(_body.length()));
		_statusCode = 400;
		return;
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
std::string HttpRequest::getCookie(const std::string& name) const {
	std::string cookieHeader = getHeaderValue("cookie");
	if (cookieHeader.empty()) {
		return "";
	}

	// Parse Cookie: name1=value1; name2=value2; name3=value3
	std::string searchKey = name + "=";
	size_t pos = 0;

	while (pos < cookieHeader.length()) {
		pos = cookieHeader.find(searchKey, pos);

		if (pos == std::string::npos) {
			return "";
		}

		// Check if this is an exact match (not a substring)
		// Valid if: at start OR preceded by '; '
		if (pos == 0 || cookieHeader[pos - 1] == ' ' ||
			(pos >= 2 && cookieHeader[pos - 2] == ';' && cookieHeader[pos - 1] == ' ')) {

			// Extract value (from '=' to ';' or end of string)
			size_t valueStart = pos + searchKey.length();
			size_t valueEnd = cookieHeader.find(';', valueStart);

			if (valueEnd == std::string::npos) {
				// Last cookie or only cookie
				return cookieHeader.substr(valueStart);
			}

			return cookieHeader.substr(valueStart, valueEnd - valueStart);
		}

		// Not an exact match, keep searching
		pos += searchKey.length();
	}

	return "";
}
