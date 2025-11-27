/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 17:16:30 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/27 16:18:24 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <cstdlib>

class HttpRequest {

	public:
		HttpRequest()
			: _requestLine(),
			_body(),
			_method(),
			_path(),
			_query(),
			_version(),
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
		const std::string& getPath() const { return _path; }
		const std::string& getQuery() const { return _query; }
		const std::string& getProtocolVersion() const {return _version;}
		std::string getUri() const;

		// Headers getters
		const std::map<std::string, std::string>& getHeaders() const { return _headers; }
		const std::string& getContentType() const;
		const std::string& getConnectionType() const;
		unsigned long getContentLength() const;
		const std::string& getTransferEncoding() const;
		const std::string& getHost() const;
		std::vector<std::string> getCgiHeadersString() const;

		// Body getters
		const std::string& getBody() const { return _body; }
		unsigned long getBodyLength() const { return _body.length(); }

		// Raw data getters
		const std::string& getRequstLine() const { return _requestLine; }
		const std::string& getRawHeaders() const { return _rawHeaders; }

		// Setters
		void setRequstLine(std::string requestLine) { _requestLine = requestLine; }
		void setRawHeaders(std::string rawHeaders) { _rawHeaders = rawHeaders; }
		void setMethod(std::string method) { _method = method; }
		void setPath(std::string path) { _path = path; }
		void setProtocolVersion(std::string version) { _version = version; }

		// Status
		bool getStatus() const { return _isValid; }

	private:
		// Raw data
		std::string	_requestLine;
		std::string	_rawHeaders;
		std::string	_body;

		// Parsed request line
		std::string _method;
		std::string	_path;
		std::string	_query;
		std::string	_version;

		// Parsed headers
		std::map<std::string, std::string>	_headers;

		// State
		bool	_isValid;

		// Internal parsing helpers
		void extractLineHeaderBodyLen(const std::string rawData);
		void parseRequestLine();
		void parseHeaders();
		const std::string& getHeaderValue(const std::string& key, const std::string& defaultValue = "") const;
};

#endif
