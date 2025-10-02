/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_request.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:16:30 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/02 14:13:11 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST
#define HTTP_REQUEST

#include <iostream>
#include <string>
#include <map>
#include <sstream>

struct ClientInfo;

enum ParseState {PARSE_REQUEST_LINE, PARSE_HEADERS, PARSE_BODY, PARSE_COMPLETE};

class HttpRequest{

	public:
		HttpRequest();
		~HttpRequest();

		void parseRequest(std::string requestData);

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

		void setRequstLine(std::string requestLine);
		void setBody(std::string body);
		void setRawHeaders(std::string rawHeaders);
		void setContentLength(unsigned long contentLength);

		//parse (get, set)
		std::string getMethod() const;
		std::string getPath() const;
		std::string getVersion() const;
		std::string getContenType() const;
		const std::map<std::string, std::string>& getHeaders() const;

		void setMethod(std::string method);
		void setPath(std::string path);
		void setVersion(std::string version);
		void setContentType(std::string ContentType);

		bool getStatus() const;

	private:
		std::string		_requestLine;
		std::string		_body;
		std::string		_rawHeaders;
		unsigned long	_contentLength;

		//reqest line
		std::string	_method;
		std::string	_path;
		std::string	_version;

		//headers
		std::map<std::string, std::string>	_headers;
		// std::string query;
		bool _isValid;
};
#endif

