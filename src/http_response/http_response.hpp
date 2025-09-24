/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_response.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/09/24 17:45:22 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTTP_RESPONSE
#define HTTTP_RESPONSE

#include <iostream>
#include <string>
#include <map>
#include <fstream>

enum fileExtentions{
	HTML,
	PDF,
	JPEG,
	TXT,
	PNG
}

class HttpResponse {

	public:
		HttpResponse(int statusCode);
		HttpResponse(int statusCode, std::string filePath);
		~HttpResponse();

		void generateResponse();
		void setBody(std::string body);
		void setReasonPhrase(std::string reasonPhrase);
		void setVersion(std::string version);
		void setStatusCode(std::string code);
		void setHeader(std::string header);

		std::string getBody();
		std::string getPath();
		std::string getVersion();
		std::string getStatusCode();
		std::string getContentType();
		char getContentLength();
		std::string getTimeNow();
		fileExtentions getFileExtension(std::string filePath);

	private:
		int _statusCode;
		std::string	_reasonPhrase;
		std::string	_date;
		std::string _serverName;
		float	_version;
		std::string _filePath;
		std::string _contentType;
		char _contentLength;
		std::string	_body;
		std::map<std::string, std::string> _headers;
};

#endif
