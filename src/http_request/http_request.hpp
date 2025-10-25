/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_request.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:16:30 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/23 17:07:18 by antonsplavn      ###   ########.fr       */
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
		std::string getContentType() const;
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
		std::string		_body; //TODO: implement unchunked body parsing
		std::string		_rawHeaders;

		//reqest line
		std::string	_method; // "GET"

		Methods		_methodEnum; //  GET

		//TODO: deliver all this vars correctly :
        // http://localhost:8080/cgi/test_POST_GET.py/foo/bar?x=1&y=2
		//http request
		std::string	_requestedPath; 	// "/cgi/test_POST_GET.py/foo/bar?x=1&y=2"  (raw path + query as received)
        std::string _queryString; 		// "x=1&y=2"                               (everything after '?')
		std::string	_version;  			// "HTTP/1.1"
        std::string _normalizedReqPath; // "/cgi/test_POST_GET.py/foo/bar"         (normalized path, no query, no duplicate slashes) ?????
		//server
        std::string	_mappedPath; 		// "/var/www/cgi/test_POST_GET.py"         (filesystem mapping for the script/resource)
        std::string _pathInfo;   		// "/foo/bar"                              (info after the script name for CGI)
        std::string _pathTranslated; 	// "/var/www/cgi/foo/bar"                  (pathInfo translated to filesystem)

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

