/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/11/27 16:15:56 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <map>
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
		const std::string& getBody() const;
		const std::string& getPath() const;
		const std::string& getProtocolVersion() const;
		int getStatusCode() const;
		unsigned long getContentLength() const;
		const std::string& getResponse() const;

	// Cookie methods
		void setCookie(const std::string& name, const std::string& value,
					int maxAge = 0, const std::string& path = "/",
					bool httpOnly = false, bool secure = false);
		void clearCookies() { _setCookies.clear(); }

	private:
		void buildHttpResponse();
		void generateErrorResponse();

		std::string	getTimeNow();
		std::string	extractFileExtension(std::string filePath);
		std::string	getReasonPhrase();
		std::string	getContentType();

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

	//body
	std::string		_body;

	//response
	std::string		_response;

	//error pages
	std::string		_customErrorPagePath;

	//cookies
	std::vector<std::string> _setCookies;
};

#endif
