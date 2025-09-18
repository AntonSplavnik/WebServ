/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_response.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/18 14:10:34 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/09/18 15:44:03 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTTP_RESPONSE
#define HTTTP_RESPONSE

#include <iostream>
#include <string>

class HttpResponse {

	public:
		HttpResponse();
		~HttpResponse();

		void setBody();
		void serReasonPhrase();
		void setVersion();
		void serStatusCode();
		void setHeader();

	private:
		std::string _body;
		std::string _reasonPhrase;
		std::string _version;
		std::string _statusCode;
		std::string _setContentType;
		std::map<std::string, std::string> _headers;
};

#endif
