
#include "response.hpp"

HttpResponse::HttpResponse(const HttpRequest& request)
	:_request(request),
	 _method(),
	 _protocolVer("HTTP/1.1 "),
	 _serverName("WebServ"),
	 _serverVersion(1.0f){}

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

std::string HttpResponse::getContentType() {
	std::string extension = extractFileExtension(_filePath);

	std::map<std::string, std::string>::const_iterator it = _mimeTypes.find(extension);
	if (it != _mimeTypes.end()) {
		return it->second;
	}
	return "application/octet-stream";
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



void HttpResponse::generateResponse(int statusCode, const std::string& cgiOutput) {
	_statusCode = statusCode;
	_reasonPhrase = getReasonPhrase();
	_date = getTimeNow();

	if (_statusCode >= 400) {
		generateErrorResponse();
		return;
	}

	if (cgiOutput.find("Content-Type:") != std::string::npos) {
		_response = _protocolVer + std::to_string(_statusCode) + " " + _reasonPhrase + "\r\n" + cgiOutput;
		return;
	} else {
		_body = cgiOutput;
		_contentLength = _body.length();
		_contentType = "text/html";
		_connectionType = _request.getConnectionType();
	}

	buildHttpResponse();
}

void HttpResponse::generateResponse(int statusCode) {
	_statusCode = statusCode;
	_reasonPhrase = getReasonPhrase();
	_date = getTimeNow();

	if (_statusCode >= 400) {
		generateErrorResponse();
		return;
	}

	_method = _request.getMethodEnum();
	_connectionType = _request.getConnectionType();

	if (_method == GET ) {
		_contentType = getContentType(); // path needed - setup externally via setPath()
		_contentLength = _body.length();
	} else if (_method == DELETE || _method == POST) {
		_body = "";
		_contentType = "text/plain";
		_contentLength = 0;
	}

	buildHttpResponse();
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

	buildHttpResponse();
}
void HttpResponse::buildHttpResponse() {

	std::ostringstream oss;

	oss << _protocolVer << _statusCode << " " << _reasonPhrase << "\r\n";
	oss << "Date: " << _date << "\r\n"
		<< "Server: " << _serverName << _serverVersion << "\r\n"
		<< "Content-Type: " << _contentType << "\r\n"
		<< "Content-Length: " << _contentLength << "\r\n"
		<< "Connection: " << _connectionType << "\r\n\r\n";

	if (!_body.empty()) {
		oss << _body;
	}
	_response = oss.str();
}

void HttpResponse::setCustomErrorPage(const std::string& errorPagePath) {
    _customErrorPagePath = errorPagePath;
}

void HttpResponse::setBody(const std::string& body) {_body = body;}
void HttpResponse::setReasonPhrase(const std::string& reasonPhrase){_reasonPhrase = reasonPhrase;}
void HttpResponse::setVersion(float version) {_serverVersion = version;}
void HttpResponse::setStatusCode(int statusCode) {_statusCode = statusCode;}
void HttpResponse::setPath(const std::string& path) {_filePath = path;}


unsigned long HttpResponse::getContentLength() const {return _body.length();}
const std::string& HttpResponse::getBody() const {return _body;}
const std::string& HttpResponse::getPath()const {return _filePath;}
float HttpResponse::getVersion() const {return _serverVersion;}
int HttpResponse::getStatusCode() const {return _statusCode;}
const std::string& HttpResponse::getResponse() const {return _response;}
