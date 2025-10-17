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
#include "../http_request/http_request.hpp"
#include <algorithm>


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
		void generateGetResponse();
		void generateDeleteResponse();
		void generateCgiResponse(const std::string &cgiOutput);

		std::string extractBody();
		std::string	getTimeNow();
		fileExtentions	extractFileExtension(std::string filePath);
		std::string	getReasonPhrase();
		std::string	getContentType();

		void setBody(std::string body);
		void setReasonPhrase(std::string reasonPhrase);
		void setVersion(float version);
		void setStatusCode(int code);
		void setHeader(std::string header);
		void setPath(std::string path);

		std::string		getBody() const;
		std::string		getPath() const;
		float			getVersion() const;
		int				getStatusCode() const;
		unsigned long	getContentLength() const;
		std::string		getResponse() const;


	private:

		void generatePostResponse();


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

		//body
		std::string	_body;

		//responce
		std::string _response;
};

#endif
