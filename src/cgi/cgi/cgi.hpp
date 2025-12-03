/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/27 13:07:52 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/28 17:24:42 by antonsplavn      ###   ########.fr       */
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

#include "request.hpp"
#include "config.hpp"

class EventLoop;
class Connection;

enum CgiState{
	CGI_CONTINUE,
	CGI_READY,
	CGI_ERROR
};

class Cgi {
	public:
		Cgi(EventLoop& eventLoop, const HttpRequest& request, int connectionFd);
		Cgi(const Cgi& other);
		~Cgi();

		bool start(const Connection& onnection);

		CgiState handleReadFromCGI();
		CgiState handleWriteToCGI();

		void cleanup();
		void closeInFd();
		void closeOutFd();
		void terminate();

		int getInFd() const { return _inFd; }
		int getOutFd() const { return _outFd; }
		int getPid() const { return _pid; }
		int getClientFd() const { return _connectionFd; };

		time_t getStartTime() const { return _startTime; }
		const HttpRequest& getRequest() const { return _request; }
		const std::string& getResponseData() const { return _responseData; }
		size_t getBytesWrittenToCgi() const {return _bytesWrittenToCgi; }

		bool isFinished() const {return _finished;}
		static std::string extractCgiExtension(const std::string& path, const LocationConfig* location);

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
		std::string findInterpreter(const std::string& name);
		char** prepEnvVariables(const Connection& onnection, const std::string& ext);
		bool chdirToScriptDir(const std::string& mappedPath);
};

#endif
