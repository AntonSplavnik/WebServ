#include <cctype>
#include <sstream>
#include "cgi_executor.hpp"
#include "connection.hpp"
#include "connection_pool_manager.hpp"
#include "response.hpp"
#include "session_manager.hpp"
#include "logger.hpp"
#include <poll.h>

void CgiExecutor::handleCGI(Connection& connection, SessionManager& sessionManager) {

	setupSession(connection, sessionManager);

	Cgi cgi(_eventLoop, connection.getRequest() , connection.getFd());
	if (!cgi.start(connection, sessionManager)){
		connection.setStatusCode(500);
		connection.prepareResponse();
		return;
	}

	//Register CGI process
	int inFd = cgi.getInFd();
	int outFd = cgi.getOutFd();
	int pid = cgi.getPid();
	if (inFd >= 0) _cgi.insert(std::make_pair(inFd, cgi));
	_cgi.insert(std::make_pair(outFd, cgi));
	connection.setState(WAITING_CGI);

	logDebug("Spawned CGI pid = " + toString(pid) + " inFd = " + toString(inFd) + " outFd = " + toString(outFd) + " for client FD = " + toString(connection.getFd()));
}

void CgiExecutor::handleCGIevent(int fd, short revents, ConnectionPoolManager& connectionPoolManager, SessionManager& sessionManager) {

	std::map<int, Cgi>::iterator cgiIt = _cgi.find(fd);
	int clientFd = cgiIt->second.getClientFd();

	Connection* connection = connectionPoolManager.getConnection(clientFd);

	if (!connection) {
		logDebug("Client disconnected, terminating CGI FD " + toString(fd));
		terminateCGI(cgiIt->second);
		return;
	}

	logDebug("Event detected on CGI FD " + toString(fd));
	if (revents & POLLERR) {
		logDebug("CGI POLLERR event on FD " + toString(fd));
		handleCGIerror(*connection, cgiIt->second, sessionManager, fd);
	} else if (revents & POLLHUP) { // On Linux, POLLHUP may occur without POLLIN even when data is available. Always try to read remaining data before treating as error
		logDebug("CGI POLLHUP event on FD " + toString(fd));
		handleCGIread(*connection, cgiIt->second, sessionManager);
		return;
	} else if (revents & POLLOUT) {
		logDebug("CGI POLLOUT event on FD " + toString(fd));
		handleCGIwrite(*connection, cgiIt->second);
	} else if (revents & POLLIN) {
		logDebug("CGI POLLIN event on FD " + toString(fd));
		handleCGIread(*connection, cgiIt->second, sessionManager);
	}

}

void CgiExecutor::handleCGIerror(Connection& connection, Cgi& cgi, SessionManager& sessionManager, int cgiFd) {

	int inFd = cgi.getInFd();
	int outFd = cgi.getOutFd();

	if (cgiFd == outFd) {

		logWarning("CGI OutFd error");

		if(cgi.isFinished()){
			logDebug("POLLERR on completed CGI, data is valid");
			updateSessionFromCGI(cgi.getResponseData(), sessionManager, connection.getSessionId());
			connection.setStatusCode(200);
			connection.prepareResponse(cgi.getResponseData(), sessionManager);
		} else {
			logDebug("POLLERR on incompleted CGI, data is currupted");
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

		logWarning("CGI inFd error");

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
void CgiExecutor::handleCGIread(Connection& connection, Cgi& cgi, SessionManager& sessionManager) { /* read from CGI */

	int inFd = cgi.getInFd();
	int outFd = cgi.getOutFd();

	CgiState status = cgi.handleReadFromCGI();
	if(status == CGI_CONTINUE) return;

	int connectionFd = cgi.getClientFd();

	if(status == CGI_READY) {
		// Update session based on CGI output
		updateSessionFromCGI(cgi.getResponseData(), sessionManager, connection.getSessionId());

		connection.setStatusCode(200);
		connection.prepareResponse(cgi.getResponseData(), sessionManager);
		logDebug("Switched client FD " + toString(connectionFd) + " to POLLOUT mode after CGI complete");
	}
	else if(status == CGI_ERROR) {
		connection.setStatusCode(500);
		connection.prepareResponse();
		logDebug("Switched client FD " + toString(connectionFd) + " to POLLOUT mode after CGI ERROR");
	}

	logDebug("Erasing CGI FD " + toString(outFd) + " from _cgi");

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
		logWarning("CGI TIMEOUT: Response 504 sent to Connection FD = " + toString(connectionFd));
	}
	logWarning("CGI TIMEOUT: CGI terminated Connection FD = " + toString(connectionFd));
	terminateCGI(cgi);

}

bool CgiExecutor::isCGI(int fd) {
	return _cgi.find(fd) != _cgi.end();
}

void CgiExecutor::setupSession(Connection& connection, SessionManager& sessionManager) {
	std::string cookieValue = connection.getRequest().getCookie("SESSID");
	std::string sessionId = sessionManager.getOrCreateSession(cookieValue);
	connection.setSessionId(sessionId);
}

// Case-insensitive string comparison function (like strncasecmp)
static int strncasecmp_custom(const std::string& s1, const std::string& s2, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (i == s1.length() || i == s2.length()) {
            return s1.length() - s2.length();
        }
        char c1 = std::tolower(static_cast<unsigned char>(s1[i]));
        char c2 = std::tolower(static_cast<unsigned char>(s2[i]));
        if (c1 != c2) {
            return c1 - c2;
        }
    }
    return 0;
}
// Case-insensitive comparison for chars
// static bool icasecmp_char(char c1, char c2) {
//     return std::tolower(static_cast<unsigned char>(c1)) == std::tolower(static_cast<unsigned char>(c2));
// }
void CgiExecutor::updateSessionFromCGI(const std::string& cgiOutput, SessionManager& sessionManager, const std::string& sessionId) {
    if (cgiOutput.empty() || sessionId.empty()) {
        return;
    }

    std::string headers;
    size_t headerEnd = cgiOutput.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = cgiOutput.find("\n\n");
        if (headerEnd != std::string::npos) {
            headers = cgiOutput.substr(0, headerEnd);
        } else {
            headers = "";
        }
    } else {
        headers = cgiOutput.substr(0, headerEnd);
    }

    std::istringstream headerStream(headers);
    std::string line;
    while (std::getline(headerStream, line)) {
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line = line.substr(0, line.length() - 1);
        }

        std::string headerKey = "X-Session-Set:";
        if (line.size() > headerKey.size() && strncasecmp_custom(line, headerKey, headerKey.size()) == 0) {
            std::string sessionVar = line.substr(headerKey.size());

            size_t start = sessionVar.find_first_not_of(" \t");
            if (start != std::string::npos) {
                sessionVar = sessionVar.substr(start);
            }

            size_t equalsPos = sessionVar.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = sessionVar.substr(0, equalsPos);
                std::string value = sessionVar.substr(equalsPos + 1);

                size_t key_end = key.find_last_not_of(" \t");
                if (key_end != std::string::npos) {
                    key = key.substr(0, key_end + 1);
                } else {
                    key.clear();
                }

                size_t key_start = key.find_first_not_of(" \t");
                if (key_start != std::string::npos) {
                    key = key.substr(key_start);
                } else {
                    key.clear();
                }

                if (!key.empty()) {
                    sessionManager.set(sessionId, key, value);
                    logDebug("Session updated from CGI: " + key + "=" + value);
                }
            }
        }
    }
}

