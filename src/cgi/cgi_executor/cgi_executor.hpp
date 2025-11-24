#ifndef CGI_EXECUTOR
#define CGI_EXECUTOR

#include "cgi.hpp"
#include <map>

class CgiExecutor {

	public:
		CgiExecutor::CgiExecutor(){};
		CgiExecutor::~CgiExecutor(){};

		void handleCGI(Connection& connection);
		void handleCGIevent(int fd, short revents, ConnectionPoolManager& connectionPoolManager);
		void handleCGItimeout(Cgi& cgi);
		bool isCGI(int fd);

		std::map<int, Cgi>& getCGI() {return _cgi;};

	private:
		void handleCGIerror(Connection& connection, int cgiFd);
		void handleCGIread(Connection& connection, int cgiFd);
		void handleCGIwrite(Connection& connection, int cgiFd);

		void terminateCGI(Cgi& cgi);

		std::map<int, Cgi>		_cgi;
};

#endif
