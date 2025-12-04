#include "cgi_executor.hpp"
#include "connection.hpp"
#include "connection_pool_manager.hpp"
#include "response.hpp"
#include "../../debug.hpp"
#include <poll.h>

void CgiExecutor::handleCGI(Connection& connection) {

	Cgi cgi(_eventLoop, connection.getRequest() , connection.getFd());
	if (!cgi.start(connection)){
		connection.setStatusCode(500);
		connection.prepareResponse();
		return;
	}

	//Register CGI process
	int inFd = cgi.getInFd();
	int outFd = cgi.getOutFd();
	//int pid = cgi.getPid();
	if (inFd >= 0) _cgi.insert(std::make_pair(inFd, cgi));
	_cgi.insert(std::make_pair(outFd, cgi));
	connection.setState(WAITING_CGI);

	// DEBUG_LOG("[DEBUG] Spawned CGI pid = " << pid
	// 		<< " inFd = " << inFd
	// 		<< " outFd = " << outFd
	// 		<< " for client FD = " << connection.getFd() << std::endl);
}

void CgiExecutor::handleCGIevent(int fd, short revents, ConnectionPoolManager& connectionPoolManager) {

	std::map<int, Cgi>::iterator cgiIt = _cgi.find(fd);
	int clientFd = cgiIt->second.getClientFd();

	Connection* connection = connectionPoolManager.getConnection(clientFd);

	if (!connection) {
		DEBUG_LOG("[DEBUG] Client disconnected, terminating CGI FD " << fd << std::endl;)
		terminateCGI(cgiIt->second);
		return;
	}

	DEBUG_LOG("[DEBUG] Event detected on CGI FD " << fd << std::endl;)
	if (revents & POLLERR) {
		DEBUG_LOG("[DEBUG] CGI POLLERR event on FD " << fd << std::endl;)
		handleCGIerror(*connection, cgiIt->second, fd);
	} else if (revents & POLLHUP) {
		DEBUG_LOG("[DEBUG] CGI POLLHUP event on FD " << fd << std::endl;)
		if (revents & POLLIN) {
			handleCGIread(*connection, cgiIt->second);
			return;
		}
		connection->setStatusCode(500);
		connection->prepareResponse();
		terminateCGI(cgiIt->second);
	} else if (revents & POLLOUT) {
		DEBUG_LOG("[DEBUG] CGI POLLOUT event on FD " << fd << std::endl;)
		handleCGIwrite(*connection, cgiIt->second);
	} else if (revents & POLLIN) {
		DEBUG_LOG("[DEBUG] CGI POLLIN event on FD " << fd << std::endl;)
		handleCGIread(*connection, cgiIt->second);
	}

}

void CgiExecutor::handleCGIerror(Connection& connection, Cgi& cgi, int cgiFd) {

	int inFd = cgi.getInFd();
	int outFd = cgi.getOutFd();

	if (cgiFd == outFd) {

		std::cerr << "[WARNING] CGI OutFd error" << std::endl;

		if(cgi.isFinished()){
			DEBUG_LOG("[DEBUG] POLLERR on completed CGI, data is valid" << std::endl;)
			connection.setStatusCode(200);
			connection.prepareResponse(cgi.getResponseData());
		} else {
			DEBUG_LOG("[DEBUG] POLLERR on incompleted CGI, data is currupted" << std::endl;)
			connection.setStatusCode(500);
			connection.prepareResponse();
		}

		if(cgi.getPid() > 0){
			cgi.terminate();
		}
		cgi.cleanup();

		if(inFd >= 0) _cgi.erase(inFd);
		if(outFd >= 0) _cgi.erase(outFd);

	} else if (cgiFd == inFd) {

		std::cerr << "[WARNING] CGI inFd error" << std::endl;

		if(cgi.getRequest().getBody().size() != cgi.getBytesWrittenToCgi()) {

			connection.setStatusCode(500);
			connection.prepareResponse();

			cgi.terminate();
			cgi.cleanup();

			if(inFd >= 0) _cgi.erase(inFd);
			if(outFd >= 0) _cgi.erase(outFd);

		} else {

			cgi.closeInFd();
			_cgi.erase(inFd);
		}
	}
}
void CgiExecutor::handleCGIwrite(Connection& connection, Cgi& cgi) { /* write to CGI */
	(void)connection;

	CgiState status = cgi.handleWriteToCGI();

	if(status == CGI_CONTINUE) return;

	if(status == CGI_READY){
		int inFd = cgi.getInFd();
		if (inFd >= 0) _cgi.erase(inFd);
		cgi.closeInFd();
	}
}
void CgiExecutor::handleCGIread(Connection& connection, Cgi& cgi) { /* read from CGI */

	int inFd = cgi.getInFd();
	int outFd = cgi.getOutFd();

	CgiState status = cgi.handleReadFromCGI();
	if(status == CGI_CONTINUE) return;

	//int connectionFd = cgi.getClientFd();

	if(status == CGI_READY) {
		connection.setStatusCode(200);
		connection.prepareResponse(cgi.getResponseData());
		// DEBUG_LOG("[DEBUG] Switched client FD " << connectionFd
		// 		  << " to POLLOUT mode after CGI complete" << std::endl);
	}
	else if(status == CGI_ERROR) {
		connection.setStatusCode(500);
		connection.prepareResponse();
		// DEBUG_LOG("[DEBUG] Switched client FD " << connectionFd
		// 		  << " to POLLOUT mode after CGI ERROR" << std::endl);
	}

	DEBUG_LOG("[DEBUG] Erasing CGI FD " << outFd << " from _cgi" << std::endl);

	if (inFd >= 0) _cgi.erase(inFd);
	if (outFd >= 0) _cgi.erase(outFd);
}

void CgiExecutor::terminateCGI(Cgi& cgi) {

	int inFd = cgi.getInFd();
	int outFd = cgi.getOutFd();
	cgi.terminate();
	cgi.cleanup();
	if (inFd >= 0) _cgi.erase(inFd);
	if (outFd >= 0) _cgi.erase(outFd);
}
void CgiExecutor::handleCGItimeout(Cgi& cgi, ConnectionPoolManager& _connectionPoolManager) {

	int connectionFd = cgi.getClientFd();
	Connection* connection = _connectionPoolManager.getConnection(connectionFd);

	//  Prepare 504 Gateway Timeout response
	if(connection){
		connection->setStatusCode(504);
		connection->prepareResponse();
		std::cout << "[CGI TIMEOUT] Response 504 sent to Connection FD = " << connectionFd << std::endl;
	}
	std::cout << "[CGI TIMEOUT] CGI terminated Connection FD = " << connectionFd << std::endl;
	terminateCGI(cgi);

}

bool CgiExecutor::isCGI(int fd) {
	return _cgi.find(fd) != _cgi.end();
}
