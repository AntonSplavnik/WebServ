/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_response.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/09/25 17:31:52 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTTP_RESPONSE
#define HTTTP_RESPONSE

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include "http_request.hpp"

enum fileExtentions{
	HTML,
	PDF,
	JPEG,
	TXT,
	PNG,
	UNKNOWN
};

class HttpResponse {

	public:
		HttpResponse(HttpRequest request);
		~HttpResponse();

		void generateResponse(int statusCode);
		void setBody(std::string body);
		void setReasonPhrase(std::string reasonPhrase);
		void setVersion(float version);
		void setStatusCode(int code);
		void setHeader(std::string header);

		std::string getBody();
		std::string getPath();
		std::string getVersion();
		std::string getStatusCode();
		std::string getReasonPhrase();
		std::string getContentType();
		long long	getContentLength();
		std::string getTimeNow();
		std::string getResponse();
		fileExtentions getFileExtension(std::string filePath);

	private:
		HttpRequest _request;

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
		long long		_contentLength;
		std:: string	_connectionType;
		std::map<std::string, std::string> _headers;

		//body
		std::string	_body;

		std::string _response;

};

#endif
