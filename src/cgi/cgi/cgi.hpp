/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/27 13:07:52 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/24 02:49:28 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
#define CGI_HPP

#define CGI_TIMEOUT 20  // seconds — how long CGI may run before it's killed TODO: increase

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
		Cgi(EventLoop& eventLoop, const HttpRequest &req, const Connection& connection,
			ConfigData& config, const LocationConfig* loc, std::string &path, std::string cgiExt);
		~Cgi();

		bool start();

		CgiState handleReadFromCGI();
		CgiState handleWriteToCGI();

		void cleanup();
		void closeInFd();
		void closeOutFd();
		void terminate();

		int getInFd() const;
		int getOutFd() const;
		int getPid() const;
		int getClientFd() const;
		const HttpRequest& getRequest() const;
		const std::string& getResponseData() const;
		time_t getStartTime() const;
		size_t getBytesWrittenToCgi() const;
		bool isFinished() const;

	private:
		pid_t						_pid;
		int							_inFd;
		int							_outFd;
		bool						_finished;
		time_t						_startTime;
		size_t						_bytesWrittenToCgi;

		std::string					_ext;
		std::string					_scriptPath;
		std::string					_resonseData;


		HttpRequest					_request;
		const Connection&			_connection;
		const LocationConfig*		_matchedLoc;
		const ConfigData&			_config;
		EventLoop&					_eventLoop;

		void prepEnv(const HttpRequest &request, const std::string &scriptPath);
		bool chdirToScriptDir();
		void executeCGI();
};


#endif
