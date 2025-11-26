/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/27 13:07:52 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/26 02:49:52 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
#define CGI_HPP

#define CGI_TIMEOUT 40  // seconds — how long CGI may run before it's killed TODO: increase

#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <vector>
#include <iostream>
#include <map>

#include "http_request.hpp"
#include "client_info.hpp"
#include "config.hpp"
#include "event_loop.hpp"

enum CgiState{
	CGI_CONTINUE,
	CGI_READY,
	CGI_ERROR
};

class Cgi {
	public:
		Cgi(EventLoop& eventLoop, const HttpRequest& request, int connectionFd);
		~Cgi();

		bool start(const Connection& onnection);

		CgiState handleReadFromCGI();
		CgiState handleWriteToCGI();

		void cleanup();
		void closeInFd();
		void closeOutFd();
		void terminate();

		int Cgi::getInFd() const { return _inFd; }
		int Cgi::getOutFd() const { return _outFd; }
		int Cgi::getPid() const { return _pid; }
		int Cgi::getClientFd() const { return _connectionFd; };

		time_t Cgi::getStartTime() const { return _startTime; }
		const HttpRequest& Cgi::getRequest() const { return _request; }
		const std::string& Cgi::getResponseData() const { return _responseData; }
		size_t Cgi::getBytesWrittenToCgi() const {return _bytesWrittenToCgi; }

		bool Cgi::isFinished() const {return _finished;}

	private:
		pid_t				_pid;
		int					_inFd;
		int					_outFd;
		bool				_finished;
		time_t				_startTime;
		size_t				_bytesWrittenToCgi;

		int					_connectionFd;
		std::string			_responseData;

		const HttpRequest&	_request;
		EventLoop&			_eventLoop;

		void executeCGI(const Connection& onnection);
		char** prepEnvVariables(const Connection& onnection, const std::string& ext);
		bool chdirToScriptDir(const std::string& mappedPath);
		std::string extractCgiExtension(const std::string& path, const LocationConfig* location);
};

#endif
