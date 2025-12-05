/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/11/29 14:30:22 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>

#include "request_router.hpp"

class HttpResponse {

	public:
		HttpResponse();
		~HttpResponse();

		void generateResponse(int statusCode);
		void generateResponse(int statusCode, const std::string& cgiOutput);

		void setProtocolVersion(const std::string& protVer);
		void setBody(const std::string& body);
		void setReasonPhrase(const std::string& reasonPhrase);
		void setStatusCode(int code);
		void setPath(const std::string& path);
		void setCustomErrorPage(const std::string& errorPagePath);
		void setConnectionType(const std::string& connectionType);
		void setMethod(RequestType type);
		void setRedirectUrl(const std::string& url);
		void addCookie(const std::string& cookieString);
		const std::string& getBody() const;
		const std::string& getPath() const;
		const std::string& getProtocolVersion() const;
		int getStatusCode() const;
		unsigned long getContentLength() const;
		const std::string& getResponse() const;


	private:
		void buildHttpResponse();
		void generateErrorResponse();

		std::string	generateCurrentTime();
		std::string	extractFileExtension(std::string filePath);
		std::string	mapStatusToReason();
		std::string	determineContentType();

		void parseCgiOutput(const std::string& cgiOutput, std::string& headers, std::string& body);
		void parseCgiContentType(const std::string& cgiHeaders);
		void parseCgiSetCookie(const std::string& cgiHeaders);
		void parseCgiLocation(const std::string& cgiHeaders);
		void parseCgiStatus(const std::string& cgiHeaders);

		static std::map<std::string, std::string> initMimeTypes();
		static const std::map<std::string, std::string> _mimeTypes;

		RequestType		_requestType;

		//status line
		std::string		_protocolVersion;
		int				_statusCode;
		std::string		_reasonPhrase;

		//headers
		std::string		_date;
		std::string		_serverName;
		float			_serverVersion;
		std::string		_filePath;
		std::string		_contentType;
		unsigned long	_contentLength;
		std::string		_connectionType;

		//CGI headers
		std::vector<std::string>	_setCookies;
		std::string					_redirectUrl;
		int							_cgiStatus;

		//body
		std::string		_body;

		//response
		std::string		_response;

		//error pages
		std::string		_customErrorPagePath;
};

#endif
