#include "cgi_executor.hpp"

void CgiExecutor::handleCGI(Connection& connection) {
	Cgi cgi(_eventLoop, httpRequest, _clients[fd], mappedPath, matchedLoc, cgiExt);
	if (!cgi.start()){
		connection.setStatusCode(/* 400? */);
		connection.prepareResponse();
	}

	//Register CGI process
	int inFd= cgi.getInFd();
	int outFd = cgi.getOutFd();
	if (inFd >= 0) _cgi[inFd] = cgi;
	_cgi[outFd] = cgi;
	_clients[fd].state = WAITING_CGI;

	std::cout << "[DEBUG] Spawned CGI pid = " << cgi.getPid()
			<< " inFd = " << inFd
			<< " outFd = " << outFd
			<< " for client FD = " << connection.getFd() << std::endl;
}

void CgiExecutor::handleCGIevent(int fd, short revents, ConnectionPoolManager& connectionPoolManager) {

	std::map<int, Cgi>::iterator cgiIt = _cgi.find(fd);
	int clientFd = cgiIt->second.getClientFd();

	_conPoolManager = &connectionPoolManager;
	Connection* connection = _conPoolManager->getConnection(clientFd);

	if (!connection) { // check if client is still there
		std::cout << "[DEBUG] Event detected on CGI FD " << fd << std::endl;
		if (revents & POLLERR) {
			std::cerr << "[DEBUG] CGI POLLERR event on FD " << fd << std::endl;
			handleCGIerror(*connection, fd);
		} else if (revents & POLLHUP) {
			std::cout << "[DEBUG] CGI POLLHUP event on FD " << fd << std::endl;
			if (revents & POLLIN) {
				handleCGIread(*connection, fd);  // final read + cleanup
				return;
			}
			// Send error response if client connection still exists
			terminateCGI(cgiIt->second);
		} else if (revents & POLLOUT) {
			std::cout << "[DEBUG] CGI POLLOUT event on FD " << fd << std::endl;
			handleCGIwrite(*connection, fd);
		}
		} else if (revents & POLLIN) {
			std::cout << "[DEBUG] CGI POLLIN event on FD " << fd << std::endl;
			handleCGIread(*connection, fd);
		return;
	} else {
		terminateCGI(cgiIt->second);
		return;
	}

}

void CgiExecutor::handleCGIerror(Connection& connection, int cgiFd) {

	Cgi& cgi = _cgi[fd];
	int connectionFd = cgi.getClientFd();
	int inFd = cgi.getInFd();
	int outFd = cgi.getOutFd();

	std::map<int, Connection>& connectionPool = _conPoolManager->getConnectionPool();

	if (fd == outFd) {

		std::cerr << "[WARNING] CGI OutFd error" << std::endl;

		if (connectionPool.find(connectionFd) != connectionPool.end()) {
			HttpResponse response (cgi.getRequest());
			if(cgi.isFinished()){
				std::cout << "[DEBUG] POLLERR on completed CGI, data is valid" << std::endl;
				response.generateResponse(200, true, cgi.getResponseData());
				connectionPool[connectionFd].setResponseData(response.getResponse());
			} else {
				std::cout << "[DEBUG] POLLERR on incompleted CGI, data is currupted" << std::endl;
				response.generateResponse(500);
				connectionPool[connectionFd].setResponseData(response.getResponse());
			}
			connectionPool[connectionFd].setState(SENDING_RESPONSE);
		}

		if(cgi.getPid() > 0){
			cgi.terminate();
		}
		cgi.cleanup();

		if(inFd >= 0) _cgi.erase(inFd);
		if(outFd >= 0) _cgi.erase(outFd);

	} else if (fd == inFd) {

		std::cerr << "[WARNING] CGI inFd error" << std::endl;

		if(cgi.getRequest().getBody().size() != cgi.getBytesWrittenToCgi()) {

			cgi.terminate();

			if (connectionPool.find(connectionFd) != connectionPool.end()){
				HttpResponse resp(cgi.getRequest());
				resp.generateResponse(500, false, "");
				connectionPool[connectionFd].setResponseData(resp.getResponse());
				connectionPool[connectionFd].setState(SENDING_RESPONSE);
			}

			cgi.cleanup();

			if(inFd >= 0) _cgi.erase(inFd);
			if(outFd >= 0) _cgi.erase(outFd);

		} else {

			cgi.closeInFd();
			_cgi.erase(inFd);
		}
	}
}
void CgiExecutor::handleCGIwrite(Connection& connection, int cgiFd) { /* write to CGI */

	Cgi& cgi = _cgi[fd];
	CgiState status = cgi.handleWriteToCGI();

	if(status == CGI_CONTINUE) return;

	if(status == CGI_READY){
		int inFd = cgi.getInFd();
		cgi.closeInFd();
		if (inFd >= 0) _cgi.erase(inFd);
	}
}
void CgiExecutor::handleCGIread(Connection& connection, int cgiFd) { /* read from CGI */

	Cgi& cgi = _cgi[cgiFd];
	CgiState status = cgi.handleReadFromCGI();
	if(status == CGI_CONTINUE) return;

	int connectionFd = cgi.getClientFd();

	HttpResponse response(cgi.getRequest());

	if(status == CGI_READY) {
		connection.setStatusCode(200);
		connection.setResponseData(cgi.getResponseData());
		// response.generateResponse(200, true, cgi.getResponseData());
		std::cout << "[DEBUG] Switched client FD " << connectionFd
				  << " to POLLOUT mode after CGI complete" << std::endl;
	}
	else if(status == CGI_ERROR) {
		connection.setStatusCode(200);
		// response.generateResponse(500);
		std::cout << "[DEBUG] Switched client FD " << connectionFd
				  << " to POLLOUT mode after CGI ERROR" << std::endl;
	}
	connection.prepareResponse();

	std::cout << "[Server] Erasing CGI FD " << cgiFd << " from _cgi" << std::endl;

	int inFd = cgi.getInFd();
	int outFd = cgi.getOutFd();
	if (inFd >= 0) _cgi.erase(inFd);
	if (outFd >= 0) _cgi.erase(outFd);
}

void CgiExecutor::terminateCGI(Cgi& cgi) {

	cgi.terminate();
	cgi.cleanup();
	int inFd = cgi.getInFd();
	int outFd = cgi.getOutFd();
	if (inFd >= 0) _cgi.erase(inFd);
	if (outFd >= 0) _cgi.erase(outFd);
}
void CgiExecutor::handleCGItimeout(Cgi& cgi) {

	std::map<int, Connection>& connectionPool = _conPoolManager->getConnectionPool();

	int connectionFd = cgi.getClientFd();
	//  Prepare 504 Gateway Timeout response
	HttpResponse response(cgi.getRequest());
	response.generateResponse(504);
	if (connectionPool.find(connectionFd) != connectionPool.end()) {
		connectionPool[connectionFd].setResponseData(response.getResponse());
		connectionPool[connectionFd].setState(SENDING_RESPONSE);
	}
	std::cout << "[CGI TIMEOUT] Response 504 sent to client FD = " << connectionFd << std::endl;
	terminateCGI(cgi);

	//this suppose to be more safe, but make sure that this safety is needed cos we might already do these checks earlier
/* 	std::map<int, Connection>& connectionPool = _conPoolManager.getConnectionPool();
	int connectionFd = cgi.getClientFd();
	std::map<int, Connection>::iterator cgit = connectionPool.find(connectionFd);

	//  Prepare 504 Gateway Timeout response
	HttpResponse response(cgi.getRequest());
	response.generateResponse(504);
	if (cgit != connectionPool.end()) {
		cgit->second.setResponseData(response.getResponse());
		cgit->second.setState(SENDING_RESPONSE);
	}
	std::cout << "[CGI TIMEOUT] Response 504 sent to client FD = " << connectionFd << std::endl;
	terminateCGI(cgi); */
}

bool CgiExecutor::isCGI(int fd) {
	return _cgi.find(fd) != _cgi.end();
}
