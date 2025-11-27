/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_response.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/11/26 11:14:22 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTTP_RESPONSE
#define HTTTP_RESPONSE

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <algorithm>
#include "http_request.hpp"

struct Cookie {
	std::string name;
	std::string value;
	std::string path;		// "/" par défaut
	std::string domain;		// optionnel
	int maxAge;              // -1 = session cookie
	bool httpOnly;           // true pour sécurité
	bool secure;             // true pour HTTPS uniquement
	std::string sameSite;    // "Strict", "Lax", ou "None"

	Cookie(const std::string& n, const std::string& v)
		: name(n), value(v), path("/"), maxAge(-1), 
		httpOnly(false), secure(false), sameSite("Lax") {}
};

enum fileExtentions{
	HTML,
	PDF,
	JPEG,
	TXT,
	PNG,
	JS,
	UNKNOWN
};

class HttpResponse {

	public:
		HttpResponse(HttpRequest request);
		~HttpResponse();

		void generateResponse(int statusCode);
		void generateResponse(int statusCode, bool isCgi, const std::string& cgiOutput);

		void setBody(std::string body);
		void setReasonPhrase(std::string reasonPhrase);
		void setVersion(float version);
		void setStatusCode(int code);
		void setHeader(std::string header);
		void setPath(std::string path);
		void setCustomErrorPage(const std::string& errorPagePath);

		std::string		getBody() const;
		std::string		getPath() const;
		float			getVersion() const;
		int				getStatusCode() const;
		unsigned long	getContentLength() const;
		std::string		getResponse() const;

		void addCookie(const Cookie& cookie);
		void addCookie(const std::string& name, const std::string& value);
		void deleteCookie(const std::string& name);


	private:

		void 			generateNormalResponse();
		void 			generateCgiResponse(const std::string& cgiOutput);

		void 			generateErrorResponse();
		std::string 	getErrorPagePath(int statusCode, const std::string& requestPath) const;

		bool 			_isCgiResponse;

		std::string 	extractBody();
		std::string		getTimeNow();
		std::string 	getContentType();
		fileExtentions	extractFileExtension(std::string filePath);
		std::string		getReasonPhrase();
		std::string		etContentType();

		HttpRequest 	_request;
		Methods			_method;

		std::string 	_customErrorPagePath;

		//status line
		std::string		_protocolVer;
		int				_statusCode;
		std::string		_reasonPhrase;

		//headers
		std::string		_date;
		std::string		_serverName;
		float			_serverVersion;
		std::string		_filePath;
		std::string		_contentType;
		unsigned long	_contentLength;
		std:: string	_connectionType;
		std::map<std::string, std::string> _headers;

		std::string			_body;
		std::string 		_response;

		std::vector<Cookie> _cookies;
		std::string buildSetCookieHeader(const Cookie& cookie) const;
};

#endif
