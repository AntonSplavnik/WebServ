#ifndef CGI_EXECUTOR
#define CGI_EXECUTOR

#include "cgi.hpp"
#include <map>

class CgiExecutor {

	public:
		CgiExecutor(ConnectionPoolManager& conPoolManager);
		~CgiExecutor();

		void handleCGI();

		void handleCGIevent(int fd, short revent);
		bool isCGI(int fd);
		void handleCGItimeout(Cgi* cgi);

		std::map<int, Cgi*>& getCGI() {return _cgi;};

	private:
		void handleCGIerror(int fd);
		void handleCGIread(int fd);
		void handleCGIwrite(int fd);

		void terminateCGI(Cgi* cgi);

		ConnectionPoolManager&	_conPoolManager;
		std::map<int, Cgi*>		_cgi;
};

#endif
