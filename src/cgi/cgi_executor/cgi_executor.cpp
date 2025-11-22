#include "cgi_executor.hpp"

CgiExecutor::CgiExecutor(ConnectionPoolManager& conPoolManager)
	:_conPoolManager(conPoolManager) {}
CgiExecutor::~CgiExecutor() {}

void CgiExecutor::handleCGI(Connection& connection, ) {

	Cgi* cgi = new Cgi(_controller, httpRequest, _clients[fd], mappedPath, matchedLoc, cgiExt);

	// Register CGI process
	int inFd = cgi->getInFd();
	int outFd = cgi->getOutFd();
	if (inFd >= 0) _cgi[inFd] = cgi;
	_cgi[outFd] = cgi;
	_clients[fd].state = WAITING_CGI;

	std::cout << "[DEBUG] Spawned CGI pid = " << cgi->getPid()
			<< " inFd = " << inFd
			<< " outFd = " << outFd
			<< " for client FD = " << fd << std::endl;
	std::cout << "clients state: " << _clients[fd].state << std::endl;
}

void CgiExecutor::handleCGIevent(int fd, short revents) {

	std::map<int, Cgi*>::iterator cgiIt = _cgi.find(fd);
	std::map<int, Connection>& connectionPool = _conPoolManager.getConnectionPool();

	if (cgiIt != _cgi.end()){
		if (connectionPool.find(cgiIt->second->getClientFd()) != connectionPool.end()) {
			std::cout << "[DEBUG] Event detected on CGI FD " << fd << std::endl;
			if (revents & POLLERR) {
				std::cerr << "[DEBUG] CGI POLLERR event on FD " << fd << std::endl;
				handleCGIerror(fd);
			} else if (revents & POLLHUP) {
				std::cout << "[DEBUG] CGI POLLHUP event on FD " << fd << std::endl;
				if (revents & POLLIN) {
					handleCGIread(fd);  // final read + cleanup
					return;
				}
				// Send error response if client connection still exists
				terminateCGI(cgiIt->second);
			} else if (revents & POLLOUT) {
				std::cout << "[DEBUG] CGI POLLOUT event on FD " << fd << std::endl;
				handleCGIwrite(fd);
			}
			} else if (revents & POLLIN) {
				std::cout << "[DEBUG] CGI POLLIN event on FD " << fd << std::endl;
				handleCGIread(fd);
			return;
		} else {
			terminateCGI(cgiIt->second);
			return;
		}
	}
}

void CgiExecutor::handleCGIerror(int fd) {

	Cgi* cgi = _cgi[fd];
	int connectionFd = cgi->getClientFd();
	int inFd = cgi->getInFd();
	int outFd = cgi->getOutFd();

	std::map<int, Connection>& connectionPool = _conPoolManager.getConnectionPool();

	if (fd == outFd) {

		std::cerr << "[WARNING] CGI OutFd error" << std::endl;

		if (connectionPool.find(connectionFd) != connectionPool.end()) {
			HttpResponse response (cgi->getRequest());
			if(cgi->isFinished()){
				std::cout << "[DEBUG] POLLERR on completed CGI, data is valid" << std::endl;
				response.generateResponse(200, true, cgi->getResponseData());
				connectionPool[connectionFd].setResponseData(response.getResponse());
			} else {
				std::cout << "[DEBUG] POLLERR on incompleted CGI, data is currupted" << std::endl;
				response.generateResponse(500);
				connectionPool[connectionFd].setResponseData(response.getResponse());
			}
			connectionPool[connectionFd].setState(SENDING_RESPONSE);
		}

		if(cgi->getPid() > 0){
			cgi->terminate();
		}
		cgi->cleanup();

		if(inFd >= 0) _cgi.erase(inFd);
		if(outFd >= 0) _cgi.erase(outFd);

		delete cgi;
	} else if (fd == inFd) {

		std::cerr << "[WARNING] CGI inFd error" << std::endl;

		if(cgi->getRequest().getBody().size() != cgi->getBytesWrittenToCgi()) {

			cgi->terminate();

			if (connectionPool.find(connectionFd) != connectionPool.end()){
				HttpResponse resp(cgi->getRequest());
				resp.generateResponse(500, false, "");
				connectionPool[connectionFd].setResponseData(resp.getResponse());
				connectionPool[connectionFd].setState(SENDING_RESPONSE);
			}

			cgi->cleanup();

			if(inFd >= 0) _cgi.erase(inFd);
			if(outFd >= 0) _cgi.erase(outFd);

			delete cgi;
		} else {

			cgi->closeInFd();
			_cgi.erase(inFd);
		}
	}
}
void CgiExecutor::handleCGIwrite(int fd) {

	Cgi* cgi = _cgi[fd];
	CgiStatus status = cgi->handleWriteToCGI();

	if(status == CGI_CONTINUE) return;

	if(status == CGI_READY){
		int inFd = cgi->getInFd();
		cgi->closeInFd();
		if (inFd >= 0) _cgi.erase(inFd);
	}
}
void CgiExecutor::handleCGIread(int fd) {

	Cgi* cgi = _cgi[fd];
	CgiStatus status = cgi->handleReadFromCGI();
	std::map<int, Connection>& connectionPool = _conPoolManager.getConnectionPool();

	if(status == CGI_CONTINUE) return;

	int connectionFd = cgi->getClientFd();

	HttpResponse response(cgi->getRequest());

	if(status == CGI_READY) {
		response.generateResponse(200, true, cgi->getResponseData());
		connectionPool[connectionFd].setResponseData(response.getResponse());
		std::cout << "[DEBUG] Switched client FD " << connectionFd
				  << " to POLLOUT mode after CGI complete" << std::endl;
	}
	else if(status == CGI_ERROR) {
		response.generateResponse(500);
		std::cout << "[DEBUG] Switched client FD " << connectionFd
				  << " to POLLOUT mode after CGI complete" << std::endl;
	}
	connectionPool[connectionFd].setState(SENDING_RESPONSE);

	std::cout << "[Server] Erasing CGI FD " << fd << " from _cgi" << std::endl;

	int inFd = cgi->getInFd();
	int outFd = cgi->getOutFd();
	if (inFd >= 0) _cgi.erase(inFd);
	if (outFd >= 0) _cgi.erase(outFd);
	delete cgi;
}

void CgiExecutor::terminateCGI(Cgi* cgi) {

	cgi->terminate();
	cgi->cleanup();
	int inFd = cgi->getInFd();
	int outFd = cgi->getOutFd();
	if (inFd >= 0) _cgi.erase(inFd);
	if (outFd >= 0) _cgi.erase(outFd);
	delete cgi;
}
void CgiExecutor::handleCGItimeout(Cgi* cgi) {

	std::map<int, Connection>& connectionPool = _conPoolManager.getConnectionPool();

	int connectionFd = cgi->getClientFd();
	//  Prepare 504 Gateway Timeout response
	HttpResponse response(cgi->getRequest());
	response.generateResponse(504);
	if (connectionPool.find(connectionFd) != connectionPool.end()) {
		connectionPool[connectionFd].setResponseData(response.getResponse());
		connectionPool[connectionFd].setState(SENDING_RESPONSE);
	}
	std::cout << "[CGI TIMEOUT] Response 504 sent to client FD = " << connectionFd << std::endl;
	terminateCGI(cgi);
}

bool CgiExecutor::isCGI(int fd) {
	return _cgi.find(fd) != _cgi.end();
}
