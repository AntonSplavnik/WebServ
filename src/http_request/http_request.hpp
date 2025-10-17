/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_request.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:16:30 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/15 15:16:58 by antonsplavn      ###   ########.fr       */
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
#include "../server/client_info.hpp"


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
		void partialParseRequest(const std::string requestData);
		
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
        void setMappedPath(const std::string& mapped) { _mappedPath = mapped; }

		//parse (get, set)
		std::string getMethod() const;
		std::string getVersion() const;
		std::string getContenType() const;
		const std::map<std::string, std::string>& getHeaders() const;
        std::string getNormalizedReqPath() const { return _normalizedReqPath; }
        std::string getMappedPath() const { return _mappedPath; }
        std::string getQueryString() const { return _queryString; }

		void setMethod(std::string method);
		void setPath(std::string path);
		void setVersion(std::string version);
		void setContentType(std::string ContentType);

		Methods getMethodEnum() const;
		bool getStatus() const;

	private:
		std::string		_requestLine;
		std::string		_body;
		std::string		_rawHeaders;

		//reqest line
		std::string	_method; // "GET"

		Methods		_methodEnum; //  GET
		std::string	_requestedPath; //  /api/v1/resource?id=123
        std::string _normalizedReqPath;     // /api/v1/resource
        std::string	_mappedPath; // /var/www/html/api/v1/resource
                                     //TODO: mapPath outside of this class
        std::string _queryString; // id=123
		std::string	_version; // HTTP/1.1

		//headers
		std::map<std::string, std::string>	_headers;
				//"Host": "example.com",
				//"Content-Type": "application/json",
				//"User-Agent": "curl/7.68.0"
		bool _isValid;
        unsigned long	_contentLength; //  348

		void extractQueryString();
        void normalizeReqPath();
};

#endif

