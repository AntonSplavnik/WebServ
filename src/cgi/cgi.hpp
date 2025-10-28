/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/27 13:07:52 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/10/28 23:42:16 by antonsplavn      ###   ########.fr       */
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
#include "server_controller.hpp"

enum CgiStatus{
	CGI_CONTINUE,
	CGI_READY,
	CGI_ERROR
};

class Cgi {
	public:
		Cgi(const std::string &path, const HttpRequest &req, std::map<int, ClientInfo> &clients,
			int clientFd, const LocationConfig* loc, std::string cgiExt, ServerController& controller);
		~Cgi();

		bool startCGI();

		CgiStatus handleReadFromCGI();
		CgiStatus handleWriteToCGI();

		void cleanup();
		void closeInFd();
		void closeOutFd();
		void terminate();

		void setMatchedLocation(const LocationConfig* loc);

		int getInFd() const;
		int getOutFd() const;
		int getPid() const;
		int getClientFd() const;
		time_t getStartTime() const;
		std::string getResponseData() const;
		HttpRequest getRequest() const;
		size_t getBytesWrittenToCgi() const;
		const LocationConfig* getMatchedLocation() const;
		bool isFinished() const;

	private:
		std::string					_ext;
		std::string					_scriptPath;
		std::string					_resonseData;
		time_t						_startTime;

		size_t						_bytesWrittenToCgi;
		pid_t						_pid;
		int							_inFd;
		int							_outFd;
		int							_clientFd;
		bool						_finished;

		HttpRequest					_request;
		std::map<int, ClientInfo>&	_clients;
		const LocationConfig*		_matchedLoc;
		ServerController&			_controller;

		void prepEnv(const HttpRequest &request, const std::string &scriptPath);
		bool chdirToScriptDir();
		void executeCGI();
};


#endif
