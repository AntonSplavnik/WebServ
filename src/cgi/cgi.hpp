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
#include "../http_request/http_request.hpp"
#include "../server/client_info.hpp"
#include "../config/config.hpp"


enum CgiReadStatus{
	CGI_CONTINUE,
	CGI_READY,
	CGI_ERROR
};

class Cgi {
	public:
		Cgi(const std::string &path, const HttpRequest &req, std::map<int, ClientInfo> &clients,
			int clientFd, const LocationConfig* loc, std::string cgiExt);
		~Cgi();

		bool startCGI();

		CgiReadStatus handleRead();
		CgiReadStatus handleWrite();

		void setMatchedLocation(const LocationConfig* loc) { _matchedLoc = loc; }

		int getClientFd() const { return _clientFd; };
		const LocationConfig* getMatchedLocation() const { return _matchedLoc; }
		time_t getStartTime() const { return startTime; }
		HttpRequest getRequest() const { return request; }
		std::string getResponseData() const { return _resonseData; }


		pid_t	pid;
		int		outFd;
		bool	finished;

	private:
		std::string					ext;
		std::string					scriptPath;
		std::string					_resonseData;
		int 						_inFd;
		int							_clientFd;
		HttpRequest					request;
		std::map<int, ClientInfo>	&_clients;
		const LocationConfig*		_matchedLoc;
		time_t						startTime;

		void setEnv(const HttpRequest &request, const std::string &scriptPath);
		bool chdirToScriptDir();
		void executeCGI();
};


#endif
