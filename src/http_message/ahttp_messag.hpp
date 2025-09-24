/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ahttp_messag.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/18 15:08:46 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/09/22 18:42:04 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef AHTTP_MESSAGE
#define AHTTP_MESSAGE

#include <iostream>
#include <string>
#include <map>

class HttpMessage{

	public:

		std::string getMethod();
		std::string getPath();
		std::string getVersion();
		std::string getHeaders();

	protected:

		std::string _line;
		std::string _headers;
		std::string _body;
		int 		_contentLength;

		std::string _path;
		std::string _version;
		// std::string query;
		std::map<std::string, std::string> _headers;
		bool _isValid;

	private:

		HttpMessage();
		~HttpMessage();

};

#endif
