#include "cgi_executor.hpp"
#include "connection.hpp"
#include "connection_pool_manager.hpp"
#include "response.hpp"
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
	int pid = cgi.getPid();
	if (inFd >= 0) {
		_cgi.insert(std::make_pair(inFd, cgi));
		std::cout << "[DEBUG] Inserted CGI into map with key inFd=" << inFd << std::endl;
	}
	_cgi.insert(std::make_pair(outFd, cgi));
	std::cout << "[DEBUG] Inserted CGI into map with key outFd=" << outFd << std::endl;
	connection.setState(WAITING_CGI);

	std::cout << "[DEBUG] Spawned CGI pid = " << pid
			<< " inFd = " << inFd
			<< " outFd = " << outFd
			<< " for client FD = " << connection.getFd() 
			<< " _cgi map size: " << _cgi.size() << std::endl;
}

void CgiExecutor::handleCGIevent(int fd, short revents, ConnectionPoolManager& connectionPoolManager) {

	std::map<int, Cgi>::iterator cgiIt = _cgi.find(fd);
	
	// Check if CGI still exists
	if (cgiIt == _cgi.end()) {
		std::cout << "[DEBUG] CGI FD " << fd << " not found in map (already terminated)" << std::endl;
		return;
	}
	
	int clientFd = cgiIt->second.getClientFd();

	Connection* connection = connectionPoolManager.getConnection(clientFd);

	if (!connection) {
		std::cout << "[DEBUG] Client disconnected, terminating CGI FD " << fd << std::endl;
		
		// Get FDs before terminating
		int inFd = cgiIt->second.getInFd();
		int outFd = cgiIt->second.getOutFd();
		
		// Terminate the CGI process
		cgiIt->second.terminate();
		cgiIt->second.cleanup();
		
		// CRITICAL: Erase using the FD we received (fd parameter), not the CGI's FDs
		// because terminate()/cleanup() may have already set them to -1
		_cgi.erase(fd);
		
		// Also erase the other FD if it exists and is different
		if (inFd >= 0 && inFd != fd) _cgi.erase(inFd);
		if (outFd >= 0 && outFd != fd) _cgi.erase(outFd);
		
		std::cout << "[DEBUG] Terminated CGI, erased fd=" << fd << " inFd=" << inFd << " outFd=" << outFd 
		          << " map size now: " << _cgi.size() << std::endl;
		return;
	}

	std::cout << "[DEBUG] Event detected on CGI FD " << fd << std::endl;
	if (revents & POLLERR) {
		std::cerr << "[DEBUG] CGI POLLERR event on FD " << fd << std::endl;
		handleCGIerror(*connection, cgiIt->second, fd);
	} else if (revents & POLLHUP) {
		std::cerr << "[DEBUG] CGI POLLHUP event on FD " << fd << std::endl;
		// Try to read any remaining data
		CgiState status = cgiIt->second.handleReadFromCGI();
		
		// Get FDs before terminating
		int inFd = cgiIt->second.getInFd();
		int outFd = cgiIt->second.getOutFd();
		
		// If CGI finished successfully, prepare response
		if (cgiIt->second.isFinished()) {
			connection->setStatusCode(200);
			connection->prepareResponse(cgiIt->second.getResponseData());
			std::cerr << "[DEBUG] CGI completed successfully on POLLHUP, status: " << static_cast<int>(status) << std::endl;
		} else {
			// Not finished - incomplete data
			connection->setStatusCode(500);
			connection->prepareResponse();
			std::cerr << "[DEBUG] CGI incomplete on POLLHUP, status: " << static_cast<int>(status) << std::endl;
		}
		
		// Terminate and cleanup
		cgiIt->second.terminate();
		cgiIt->second.cleanup();
		
		// CRITICAL: Erase using the FD we received (fd parameter)
		_cgi.erase(fd);
		
		// Also erase the other FD if it exists and is different
		if (inFd >= 0 && inFd != fd) _cgi.erase(inFd);
		if (outFd >= 0 && outFd != fd) _cgi.erase(outFd);
		
		std::cerr << "[DEBUG] POLLHUP: erased fd=" << fd << " inFd=" << inFd << " outFd=" << outFd 
		          << " map size now: " << _cgi.size() << std::endl;
	} else if (revents & POLLOUT) {
		std::cout << "[DEBUG] CGI POLLOUT event on FD " << fd << std::endl;
		handleCGIwrite(*connection, cgiIt->second);
	} else if (revents & POLLIN) {
		std::cout << "[DEBUG] CGI POLLIN event on FD " << fd << std::endl;
		handleCGIread(*connection, cgiIt->second);
	}

}

void CgiExecutor::handleCGIerror(Connection& connection, Cgi& cgi, int cgiFd) {

	int inFd = cgi.getInFd();
	int outFd = cgi.getOutFd();

	if (cgiFd == outFd) {

		std::cerr << "[WARNING] CGI OutFd error" << std::endl;

		if(cgi.isFinished()){
			std::cout << "[DEBUG] POLLERR on completed CGI, data is valid" << std::endl;
			connection.setStatusCode(200);
			connection.prepareResponse(cgi.getResponseData());
		} else {
			std::cout << "[DEBUG] POLLERR on incompleted CGI, data is currupted" << std::endl;
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

	int connectionFd = cgi.getClientFd();

	if(status == CGI_READY) {
		connection.setStatusCode(200);
		connection.prepareResponse(cgi.getResponseData());
		std::cout << "[DEBUG] Switched client FD " << connectionFd
				  << " to POLLOUT mode after CGI complete" << std::endl;
	}
	else if(status == CGI_ERROR) {
		connection.setStatusCode(500);
		connection.prepareResponse();
		std::cout << "[DEBUG] Switched client FD " << connectionFd
				  << " to POLLOUT mode after CGI ERROR" << std::endl;
	}

	std::cout << "[DEBUG] Erasing CGI FD " << outFd << " from _cgi" << std::endl;

	if (inFd >= 0) _cgi.erase(inFd);
	if (outFd >= 0) _cgi.erase(outFd);
}

void CgiExecutor::terminateCGI(Cgi& cgi) {

	int inFd = cgi.getInFd();
	int outFd = cgi.getOutFd();
	std::cout << "[DEBUG] terminateCGI called: inFd=" << inFd << ", outFd=" << outFd << std::endl;
	cgi.terminate();
	cgi.cleanup();
	if (inFd >= 0) {
		std::cout << "[DEBUG] Erasing CGI inFd " << inFd << " from map" << std::endl;
		_cgi.erase(inFd);
	}
	if (outFd >= 0) {
		std::cout << "[DEBUG] Erasing CGI outFd " << outFd << " from map" << std::endl;
		_cgi.erase(outFd);
	}
	std::cout << "[DEBUG] terminateCGI completed, _cgi map size: " << _cgi.size() << std::endl;
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
