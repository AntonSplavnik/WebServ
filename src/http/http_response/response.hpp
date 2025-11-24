/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/11/24 21:56:16 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <algorithm>
#include "http_request.hpp"

class HttpResponse {

	public:
		HttpResponse(const HttpRequest& request);
		~HttpResponse();

		void generateResponse(int statusCode);
		void generateResponse(int statusCode, const std::string& cgiOutput);

		void setBody(const std::string& body);
		void setReasonPhrase(const std::string& reasonPhrase);
		void setVersion(float version);
		void setStatusCode(int code);
		void setPath(const std::string& path);
		void setCustomErrorPage(const std::string& errorPagePath);
		void setConnectionType(const std::string& connectionType);
		const std::string& getBody() const;
		const std::string& getPath() const;
		float getVersion() const;
		int getStatusCode() const;
		unsigned long getContentLength() const;
		const std::string& getResponse() const;


	private:
		void buildHttpResponse();
		void generateErrorResponse();

		std::string	getTimeNow();
		std::string	extractFileExtension(std::string filePath);
		std::string	getReasonPhrase();
		std::string	getContentType();

		static std::map<std::string, std::string> initMimeTypes();
		static const std::map<std::string, std::string> _mimeTypes;

		const HttpRequest& _request;
		Methods			_method;

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
		std::string		_connectionType;

		//body
		std::string		_body;

		//response
		std::string		_response;

		//error pages
		std::string		_customErrorPagePath;
};

#endif
