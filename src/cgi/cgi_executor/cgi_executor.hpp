#ifndef CGI_EXECUTOR_HPP
#define CGI_EXECUTOR_HPP

#include "cgi.hpp"
#include <map>

class EventLoop;
class Connection;
class ConnectionPoolManager;

class CgiExecutor {

	public:
		CgiExecutor(EventLoop& eventLoop) : _eventLoop(eventLoop){};
		~CgiExecutor(){};

		void handleCGI(Connection& connection);
		void handleCGIevent(int fd, short revents, ConnectionPoolManager& connectionPoolManager);
		void handleCGItimeout(Cgi& cgi, ConnectionPoolManager& _connectionPoolManager);
		bool isCGI(int fd);

		std::map<int, Cgi>& getCGI() {return _cgi;};

	private:
		void handleCGIerror(Connection& connection, Cgi& cgi, int cgiFd);
		void handleCGIread(Connection& connection, Cgi& cgi);
		void handleCGIwrite(Connection& connection, Cgi& cgi);

		void terminateCGI(Cgi& cgi);

		EventLoop&			_eventLoop;
		std::map<int, Cgi>	_cgi;
};

#endif
