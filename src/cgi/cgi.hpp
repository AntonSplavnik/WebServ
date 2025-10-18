#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <vector>
#include <iostream>
#include <map>
#include "../http_request/http_request.hpp"
#include "../server/client_info.hpp"


class Cgi {
public:
	Cgi(const std::string &path,
		const HttpRequest &req,
		std::map<int, ClientInfo> &clients,
		int clientFd);
	~Cgi();

	bool	start();
	bool	handleRead();
	int		getClientFd() const { return _clientFd; };

	pid_t	pid;
	int		outFd;
	bool 	finished;

private:
	int 		inFd;
	std::string scriptPath;
	HttpRequest request;
	std::map<int, ClientInfo> &_clients;
	int			_clientFd;
	std::string buffer;
	time_t 		startTime;

	void	setEnv(const HttpRequest &request, const std::string &scriptPath);
	bool chdirToScriptDir(const std::string &scriptPath);
};


#endif
