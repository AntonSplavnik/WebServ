#ifndef CGI_HPP
#define CGI_HPP

#define CGI_TIMEOUT 3  // seconds — how long CGI may run before it's killed TODO: increase

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


class Cgi {
public:
	Cgi(const std::string &path,
		const HttpRequest &req,
		std::map<int, ClientInfo> &clients,
		int clientFd, const LocationConfig* loc, std::string cgiExt);
	~Cgi();

	bool	start();
	bool	handleRead();
	int		getClientFd() const { return _clientFd; };
	void	setMatchedLocation(const LocationConfig* loc) { _matchedLoc = loc; }
	const LocationConfig* getMatchedLocation() const { return _matchedLoc; }
    time_t getStartTime() const { return startTime; }
    HttpRequest getRequest() const { return request; }


	pid_t	pid;
	int		outFd;
	bool 	finished;

private:
	const LocationConfig* _matchedLoc;
	std::string ext;
	int 		inFd;
	std::string scriptPath;
	HttpRequest request;
	std::map<int, ClientInfo> &_clients;
	int			_clientFd;
	std::string buffer;
	time_t 		startTime;


	void	setEnv(const HttpRequest &request, const std::string &scriptPath);
	bool chdirToScriptDir();
    void executeCgiWithArgs();
};


#endif
