/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_response.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/10/14 18:39:55 by antonsplavn      ###   ########.fr       */
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
#include "config.hpp"


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
		HttpResponse(HttpRequest request, const ConfigData& configData);
		~HttpResponse();

		void generateResponse(int statusCode);

		void setBody(std::string body);
		void setReasonPhrase(std::string reasonPhrase);
		void setVersion(float version);
		void setStatusCode(int code);
		void setContentType(std::string contentType);
		void setHeader(std::string header);
		void setPath(std::string path);

		std::string		getBody() const;
		std::string		getPath() const;
		float			getVersion() const;
		int				getStatusCode() const;
		unsigned long	getContentLength() const;
		std::string		getResponse() const;
		std::string		getContentType() const;


	private:

		void generateGetResponse();
		void generatePostResponse();
		void generateDeleteResponse();
		void generateErrorResponse();

		std::string extractBody();
		std::string	getTimeNow();
		fileExtentions	extractFileExtension(std::string filePath);
		std::string	getReasonPhrase();
		std::string	determineContentType();

		std::string getErrorPagePath(int statusCode, const std::string& requestPath) const;
		// std::string loadErrorPage(const std::string& errorPagePath);

		HttpRequest _request;
		Methods		_method;

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

		const ConfigData& _config;

		//body
		std::string	_body;

		//responce
		std::string _response;
};

#endif
