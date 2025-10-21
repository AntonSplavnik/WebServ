/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_request.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:16:30 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/21 17:13:12 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST
#define HTTP_REQUEST

#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include "client_info.hpp"

enum Methods {
	GET,
	POST,
	DELETE
};

class HttpRequest{

	public:
		HttpRequest();
		~HttpRequest();

		void parseRequest(std::string requestData);
		void ParsePartialRequest(const std::string requestData);

		void extractLineHeaderBodyLen(const std::string rawData);
		void parseRequestLine();
		void parseBody();
		void parseHeaders();

		//parse
		void parseMethod();
		void parsePath();
		void parseQuery();
		void parseVersion();

		//extract (get, set)
		std::string getRequstLine() const;
		std::string getBody() const;
		std::string getRawHeaders() const;
		unsigned long getContentLength() const;
		unsigned long getBodyLength() const;

		void setRequstLine(std::string requestLine);
		void setBody(std::string body);
		void setRawHeaders(std::string rawHeaders);
		void setContentLength(unsigned long contentLength);

		//parse (get, set)
		std::string getMethod() const;
		std::string getPath() const;
		std::string getVersion() const;
		std::string getContenType() const;
		std::string getConnectionType() const;

		const std::map<std::string, std::string>& getHeaders() const;

		void setMethod(std::string method);
		void setPath(std::string path);
		void setVersion(std::string version);
		void setContentType(std::string ContentType) const;

		Methods getMethodEnum() const;
		bool getStatus() const;

	private:
		std::string		_requestLine;
		std::string		_body;
		std::string		_rawHeaders;

		//reqest line
		std::string		_method;
		Methods			_methodEnum;
		std::string		_path;
		std::string		_version;
		unsigned long	_contentLength;

		//headers
		std::map<std::string, std::string>	_headers;
		// std::string query;
		bool _isValid;
};
#endif

