/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_request.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:16:30 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/22 22:02:49 by antonsplavn      ###   ########.fr       */
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


class HttpRequest {

	public:
		HttpRequest()
			: _requestLine(),
			_body(),
			_method(),
			_path(),
			_query(),
			_version(),
			_contentLength(0),
			_headers(),
			_isValid(true) {
		}
		~HttpRequest() {}

		// Parsing methods
		void parseRequestHeaders(const std::string requestData);
		void addBody(const std::string& requestBuffer);
		void parseBody();

		// Request line getters
		const std::string& getMethod() const { return _method; }
		Methods getMethodEnum() const { return _methodEnum; }
		const std::string& getPath() const { return _path; }
		const std::string& getQuery() const { return _query; }
		const std::string& getVersion() const { return _version; }

		// Headers getters
		const std::map<std::string, std::string>& getHeaders() const { return _headers; }
		std::string getContentType() const;
		std::string getConnectionType() const;
		std::string getTransferEncoding() const;
		unsigned long getContentLength() const { return _contentLength; }

		// Body getters
		const std::string& getBody() const { return _body; }
		unsigned long getBodyLength() const { return _body.length(); }
	
		// Body setters
		void  setBody(const std::string& body) { _body = body; }		// Raw data getters
		const std::string& getRequstLine() const { return _requestLine; }
		const std::string& getRawHeaders() const { return _rawHeaders; }

		// Setters
		void setRequstLine(std::string requestLine) { _requestLine = requestLine; }
		void setRawHeaders(std::string rawHeaders) { _rawHeaders = rawHeaders; }
		void setMethod(std::string method) { _method = method; }
		void setPath(std::string path) { _path = path; }
		void setVersion(std::string version) { _version = version; }
		void setContentLength(unsigned long contentLength) { _contentLength = contentLength; }

		// Status
		bool getStatus() const { return _isValid; }

	private:
		// Raw data
		std::string	_requestLine;
		std::string	_rawHeaders;
		std::string	_body;

		// Parsed request line
		std::string _method;
		Methods		_methodEnum;
		std::string	_path;
		std::string	_query;
		std::string	_version;

		// Parsed headers
		std::map<std::string, std::string>	_headers;
		unsigned long	_contentLength;

		// State
		bool	_isValid;

		// Internal parsing helpers
		void extractLineHeaderBodyLen(const std::string rawData);
		void parseRequestLine();
		void parseHeaders();
};

#endif
