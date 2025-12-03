
#include "response.hpp"

HttpResponse::HttpResponse()
	:_statusCode(0),
	 _serverName("WebServ"),
	 _serverVersion(1.0f),
	 _contentLength(0),
	 _cgiStatus(0){}

HttpResponse::~HttpResponse(){}

const std::map<std::string, std::string> HttpResponse::_mimeTypes = HttpResponse::initMimeTypes();

std::map<std::string, std::string> HttpResponse::initMimeTypes() {
	std::map<std::string, std::string> types;

	// Text
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

	// Video
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

	// Fonts
	types["ttf"] = "font/ttf";
	types["woff"] = "font/woff";
	types["woff2"] = "font/woff2";

	return types;
}

std::string HttpResponse::extractFileExtension(std::string filePath) {
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

std::string HttpResponse::determineContentType() {
	std::string extension = extractFileExtension(_filePath);

	std::map<std::string, std::string>::const_iterator it = _mimeTypes.find(extension);
	if (it != _mimeTypes.end()) {
		return it->second;
	}
	return "application/octet-stream";
}
std::string HttpResponse::mapStatusToReason() {

	switch (_statusCode) {
		// Success
		case 200: return "OK";
		case 204: return "No Content";
		case 302: return "Found";
		case 303: return "See Other";
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
std::string HttpResponse::generateCurrentTime() {

	time_t now = time(0);
	struct tm* gmtTime = gmtime(&now);
	char buffer[100];
	strftime(buffer, 100, "%a, %d %b %Y %H:%M:%S GMT", gmtTime);
	std::string httpTime = buffer;
	return httpTime;
}

void HttpResponse::generateResponse(int statusCode, const std::string& cgiOutput) {
	_statusCode = statusCode;
	_reasonPhrase = mapStatusToReason();
	_date = generateCurrentTime();

	if (_statusCode >= 400) {
		generateErrorResponse();
		return;
	}

	std::string cgiHeaders;
	parseCgiOutput(cgiOutput, cgiHeaders, _body);

	_contentLength = _body.length();

	parseCgiContentType(cgiHeaders);
	parseCgiSetCookie(cgiHeaders);
	parseCgiLocation(cgiHeaders);
	parseCgiStatus(cgiHeaders);

	// Override status code if CGI script
	if (_cgiStatus > 0) {
		_statusCode = _cgiStatus;
		_reasonPhrase = mapStatusToReason();
	}

	buildHttpResponse();
}
void HttpResponse::generateResponse(int statusCode) {
	_statusCode = statusCode;
	_reasonPhrase = mapStatusToReason();
	_date = generateCurrentTime();

	if (_statusCode >= 400) {
		generateErrorResponse();
		return;
	}

	if (_requestType == GET || _requestType == CGI_GET || _requestType == HEAD) {
		_contentType = determineContentType(); // path needed - setup externally via setPath()
		_contentLength = _body.length();
	} else if (_requestType == DELETE || _requestType == POST || _requestType == CGI_POST) {
		_body = "";
		_contentType = "text/plain";
		_contentLength = 0;
	}

	buildHttpResponse();
}
void HttpResponse::generateErrorResponse() {

	if (!_customErrorPagePath.empty()) {
		std::ifstream errorFile(_customErrorPagePath.c_str());
		if (errorFile.is_open()) {
			std::stringstream buffer;
			buffer << errorFile.rdbuf();
			_body = buffer.str();
			errorFile.close();
		}
	}
	if (_body.empty()) {
		std::stringstream oss;
		oss << "<html><body><h1>Error " << _statusCode << "</h1></body></html>";
		_body = oss.str();
	}
	_contentType = "text/html";
	_contentLength = _body.length();

	buildHttpResponse();
}
void HttpResponse::buildHttpResponse() {

	std::ostringstream oss;

	oss << _protocolVersion << " " << _statusCode << " " << _reasonPhrase << "\r\n";
	oss << "Date: " << _date << "\r\n"
		<< "Server: " << _serverName << _serverVersion << "\r\n"
		<< "Content-Type: " << _contentType << "\r\n"
		<< "Content-Length: " << _contentLength << "\r\n";

	// Add Location header if present (for redirects)
	if (!_redirectUrl.empty()) {
		oss << "Location: " << _redirectUrl << "\r\n";
	}

	// Add Set-Cookie headers if present
	for (size_t i = 0; i < _setCookies.size(); i++) {
		oss << "Set-Cookie: " << _setCookies[i] << "\r\n";
	}

	oss << "Connection: " << _connectionType << "\r\n\r\n";

	if (_requestType != HEAD && !_body.empty()) {
		oss << _body;
	}
	_response = oss.str();

	// Debug: print response headers (limit to avoid huge output)
	std::cout << "[DEBUG] HTTP Response (" << _statusCode << "):" << std::endl;
	size_t headerEnd = _response.find("\r\n\r\n");
	if (headerEnd != std::string::npos) {
		std::cout << _response.substr(0, headerEnd + 4) << std::endl;
	} else {
		std::cout << _response << std::endl;
	}
}

void HttpResponse::setCustomErrorPage(const std::string& errorPagePath) {
    _customErrorPagePath = errorPagePath;
}

void HttpResponse::setProtocolVersion(const std::string& protVer) {_protocolVersion = protVer;}
void HttpResponse::setBody(const std::string& body) {_body = body;}
void HttpResponse::setReasonPhrase(const std::string& reasonPhrase){_reasonPhrase = reasonPhrase;}
void HttpResponse::setStatusCode(int statusCode) {_statusCode = statusCode;}
void HttpResponse::setPath(const std::string& path) {_filePath = path;}
void HttpResponse::setConnectionType(const std::string& connectionType) {_connectionType = connectionType;}
void HttpResponse::setMethod(RequestType type) {_requestType = type;}
void HttpResponse::setRedirectUrl(const std::string& url) {_redirectUrl = url;}

unsigned long HttpResponse::getContentLength() const {return _body.length();}
const std::string& HttpResponse::getBody() const {return _body;}
const std::string& HttpResponse::getPath()const {return _filePath;}
const std::string& HttpResponse::getProtocolVersion() const {return _protocolVersion;}
int HttpResponse::getStatusCode() const {return _statusCode;}
const std::string& HttpResponse::getResponse() const {return _response;}

void HttpResponse::parseCgiOutput(const std::string& cgiOutput, std::string& headers, std::string& body) {
	size_t headerEnd = cgiOutput.find("\r\n\r\n");

	if (headerEnd == std::string::npos) {
		headerEnd = cgiOutput.find("\n\n");
		if (headerEnd != std::string::npos) {
			headers = cgiOutput.substr(0, headerEnd);
			body = cgiOutput.substr(headerEnd + 2);
		} else {
			// No header separator, treat entire output as body
			headers = "";
			body = cgiOutput;
		}
	} else {
		headers = cgiOutput.substr(0, headerEnd);
		body = cgiOutput.substr(headerEnd + 4);
	}
}
void HttpResponse::parseCgiContentType(const std::string& cgiHeaders) {
	if (cgiHeaders.empty()) {
		return;
	}

	std::string line;
	size_t pos = 0;
	size_t nextPos = 0;

	while ((nextPos = cgiHeaders.find('\n', pos)) != std::string::npos) {
		line = cgiHeaders.substr(pos, nextPos - pos);

		// Remove \r if present
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line = line.substr(0, line.length() - 1);
		}

		// Check for Content-Type header (case-insensitive)
		if (line.find("Content-Type:") == 0 || line.find("Content-type:") == 0) {
			size_t colonPos = line.find(':');
			if (colonPos != std::string::npos) {
				_contentType = line.substr(colonPos + 1);
				// Trim leading whitespace
				size_t start = _contentType.find_first_not_of(" \t");
				if (start != std::string::npos) {
					_contentType = _contentType.substr(start);
				}
			}
			break;
		}

		pos = nextPos + 1;
	}
}
void HttpResponse::parseCgiSetCookie(const std::string& cgiHeaders) {
	if (cgiHeaders.empty()) {
		return;
	}

	std::string line;
	size_t pos = 0;
	size_t nextPos = 0;

	while ((nextPos = cgiHeaders.find('\n', pos)) != std::string::npos) {
		line = cgiHeaders.substr(pos, nextPos - pos);

		// Remove \r if present
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line = line.substr(0, line.length() - 1);
		}

		// Check for Set-Cookie header (case-insensitive)
		if (line.find("Set-Cookie:") == 0 || line.find("Set-cookie:") == 0) {
			size_t colonPos = line.find(':');
			if (colonPos != std::string::npos) {
				std::string cookie = line.substr(colonPos + 1);
				// Trim leading whitespace
				size_t start = cookie.find_first_not_of(" \t");
				if (start != std::string::npos) {
					cookie = cookie.substr(start);
				}
				_setCookies.push_back(cookie);
			}
		}

		pos = nextPos + 1;
	}
}
void HttpResponse::parseCgiLocation(const std::string& cgiHeaders) {
	if (cgiHeaders.empty()) {
		return;
	}

	std::string line;
	size_t pos = 0;
	size_t nextPos = 0;

	while ((nextPos = cgiHeaders.find('\n', pos)) != std::string::npos) {
		line = cgiHeaders.substr(pos, nextPos - pos);

		// Remove \r if present
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line = line.substr(0, line.length() - 1);
		}

		// Check for Location header (case-insensitive)
		if (line.find("Location:") == 0 || line.find("location:") == 0) {
			size_t colonPos = line.find(':');
			if (colonPos != std::string::npos) {
				_redirectUrl = line.substr(colonPos + 1);
				// Trim leading whitespace
				size_t start = _redirectUrl.find_first_not_of(" \t");
				if (start != std::string::npos) {
					_redirectUrl = _redirectUrl.substr(start);
				}
			}
			break;
		}

		pos = nextPos + 1;
	}
}
void HttpResponse::parseCgiStatus(const std::string& cgiHeaders) {
	if (cgiHeaders.empty()) {
		return;
	}

	std::string line;
	size_t pos = 0;
	size_t nextPos = 0;

	while ((nextPos = cgiHeaders.find('\n', pos)) != std::string::npos) {
		line = cgiHeaders.substr(pos, nextPos - pos);

		// Remove \r if present
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line = line.substr(0, line.length() - 1);
		}

		// Check for Status header (case-insensitive)
		if (line.find("Status:") == 0 || line.find("status:") == 0) {
			size_t colonPos = line.find(':');
			if (colonPos != std::string::npos) {
				std::string statusStr = line.substr(colonPos + 1);
				// Trim leading whitespace
				size_t start = statusStr.find_first_not_of(" \t");
				if (start != std::string::npos) {
					statusStr = statusStr.substr(start);
				}
				// Extract status code (first 3 digits)
				_cgiStatus = std::atoi(statusStr.c_str());
			}
			break;
		}

		pos = nextPos + 1;
	}
}

/*
 * Set session cookie with security attributes
 * 
 * @param name: Cookie name (e.g., "webserv_sid")
 * @param value: Cookie value (session ID)
 * @param maxAge: Expiration in seconds (0 = session cookie)
 * @param httpOnly: Prevent JavaScript access
 * @param secure: Only send over HTTPS
 * @param sameSite: CSRF protection ("Strict", "Lax", or "None")
 */
void HttpResponse::setCookie(const std::string& name, const std::string& value,
                             int maxAge, bool httpOnly, bool secure, const std::string& sameSite) {
	std::ostringstream oss;
	oss << name << "=" << value;

	if (maxAge > 0) {
		oss << "; Max-Age=" << maxAge;
	}

	if (httpOnly) {
		oss << "; HttpOnly";
	}

	if (secure) {
		oss << "; Secure";
	}

	if (!sameSite.empty()) {
		oss << "; SameSite=" << sameSite;
	}

	oss << "; Path=/";

	_setCookies.push_back(oss.str());
}

/*
 * Clear cookie by setting Max-Age=0
 * 
 * @param name: Cookie name to clear
 */
void HttpResponse::clearCookie(const std::string& name) {
	std::ostringstream oss;
	oss << name << "=; Max-Age=0; Path=/";
	_setCookies.push_back(oss.str());
}
