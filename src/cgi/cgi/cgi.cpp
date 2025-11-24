/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/27 13:07:59 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/11/24 12:44:17 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "cgi.hpp"
#include "http_response.hpp"
#include "logger.hpp"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <algorithm>
#include <signal.h>

#define MAX_CGI_OUTPUT (10 * 1024 * 1024)

Cgi::Cgi(EventLoop& eventLoop, const HttpRequest& req, const Connection& connection,
		ConfigData& congig, const LocationConfig* loc, std::string& path, std::string cgiExt)
        : _pid(-1),
          _inFd(-1),
          _outFd(-1),
          _finished(false),
          _startTime(time(NULL)),
          _bytesWrittenToCgi(0),
          _eventLoop(eventLoop),
          _request(req),
          _connection(connection),
          _config(congig),
          _matchedLoc(loc), // ??
          _scriptPath(path),
          _ext(cgiExt) {} // comes from config (cgi extentions)
Cgi::~Cgi() {
    cleanup();
}

bool Cgi::start() {

    //TODO: unchunk (ask if Damien makes it, so maybe i can use it here)
    int inpipe[2];
    int outpipe[2];

    if (pipe(inpipe) < 0 || pipe(outpipe) < 0) {
        perror("pipe");
        return false;
    }

	if (access(_scriptPath.c_str(), X_OK) != 0) {
    	perror("access");
    	return false; // will trigger 500
	}
    _pid = fork();
    if (_pid < 0) {
        perror("fork");
        return false;
    }

    // --- CHILD ---
    if (_pid == 0) {
        dup2(inpipe[0], STDIN_FILENO);
        close(inpipe[1]);

        dup2(outpipe[1], STDOUT_FILENO);
        close(outpipe[0]);

        {
            /*
                - RLIMIT_AS limits the total address space (virtual memory) the process can allocate
                - If the CGI tries to allocate more, malloc/new will fail and the script will crash (not your
                server)
                - The child exits with error → parent detects it via POLLHUP/POLLERR → returns 500 to client
            */
            // Limit memory usage BEFORE execution
            struct rlimit mem_limit;
            mem_limit.rlim_cur = 50 * 1024 * 1024;
            mem_limit.rlim_max = 100 * 1024 * 1024;

            if (setrlimit(RLIMIT_AS, &mem_limit) != 0) {
                perror("setrlimit");
                _exit(126);
            }
        }

        if (!chdirToScriptDir()) {
            _exit(126);  // non-zero => parent will treat as 500
        }

        prepEnv(_request, _scriptPath);  // Ensure PATH is set
        executeCGI();
    }

    // --- PARENT ---
    close(inpipe[0]);
    close(outpipe[1]);

    _inFd = inpipe[1];
    _outFd = outpipe[0];

    fcntl(_inFd, F_SETFL, O_NONBLOCK);
    fcntl(_outFd, F_SETFL, O_NONBLOCK);

    if(_request.getMethod() != "POST"){
        closeInFd();
    }

    return true;
}
void Cgi::prepEnv(const HttpRequest &request, const std::string &scriptPath) {

    //  Base CGI variables
    setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
    setenv("SERVER_PROTOCOL", request.getVersion().c_str(), 1);
    setenv("SERVER_SOFTWARE", SERVER_SOFTWARE_NAME, 1);

    //  Server name and port
    std::string host = "localhost";
    std::map<std::string, std::string> headers = request.getHeaders();
    std::map<std::string, std::string>::const_iterator it = headers.find("host");
    if (it != headers.end())
        host = it->second;
    setenv("SERVER_NAME", _config.server_names, 1);  //get from config data;
    setenv("SERVER_PORT", "8080", 1); // or from _matchedLoc / configData;

    //  Request details
    setenv("REQUEST_METHOD", request.getMethod().c_str(), 1);
    setenv("QUERY_STRING", request.getQuery().c_str(), 1);
    setenv("SCRIPT_FILENAME", scriptPath.c_str(), 1);
    setenv("SCRIPT_NAME", request.getNormalizedReqPath().c_str(), 1);
    setenv("PATH", "/usr/local/bin:/usr/bin:/bin", 1);
	setenv("DOCUMENT_ROOT", _matchedLoc ? _matchedLoc->root.c_str() : ".", 1);
	setenv("REDIRECT_STATUS", "200", 1); // required for php-cgi
	setenv("REQUEST_URI", request.getNormalizedReqPath().c_str(), 1);

    std::map<std::string, std::string>::const_iterator it = headers.begin();
    for (;it != headers.end(); ++it)
	{
    	std::string key = "HTTP_" + it->first;
    	std::transform(key.begin(), key.end(), key.begin(), ::toupper);
    	std::replace(key.begin(), key.end(), '-', '_');
    	setenv(key.c_str(), it->second.c_str(), 1);
	}

    // Content info (for POST, PUT)
    std::string body = request.getBody();
    std::string contentLength = "0";
    if (!body.empty()) {
        std::ostringstream oss;
        oss << body.size();
        contentLength = oss.str();
    }
    setenv("CONTENT_LENGTH", contentLength.c_str(), 1);
    setenv("CONTENT_TYPE", request.getContentType().c_str(), 1);

    //  Client (REMOTE_ADDR / PORT)
    std::string remoteAddr = "127.0.0.1";
    std::string remotePort = "80";
#if 0
    if (_clients.find(_clientFd) != _clients.end()) {
        remoteAddr = _clients[_clientFd].client_ip;
        std::ostringstream port;
        port << _clients[_clientFd].client_port;
        remotePort = port.str();
    }
#endif //TODO: add client ip and port to ClientInfo and set them on accept
    setenv("REMOTE_ADDR", remoteAddr.c_str(), 1);
    setenv("REMOTE_PORT", remotePort.c_str(), 1);

    //  PATH_INFO / PATH_TRANSLATED
    // (if your request URL is /cgi/test.py/foo/bar → PATH_INFO = /foo/bar)
    std::string pathInfo = ""; // TODO: extract from request path
    std::string pathTranslated = ""; // TODO: map to filesystem
    std::string reqPath = request.getNormalizedReqPath();
    // std::string scriptName = request.getMappedPath();            <=====================this is definately wrong

    std::string urlPath = request.getNormalizedReqPath();
	std::string locPath = _matchedLoc ? _matchedLoc->path : "";

	if (urlPath.size() > locPath.size())
	{
    	std::string pathInfo = urlPath.substr(locPath.size());
    	std::string pathTranslated = scriptPath + pathInfo;
    	setenv("PATH_INFO", pathInfo.c_str(), 1);
    	setenv("PATH_TRANSLATED", pathTranslated.c_str(), 1);
	}
}
void Cgi::executeCGI() {
    // --- Determine interpreter ---
    std::string interpreter;
    if (_ext == ".py")
        interpreter = "/usr/bin/python3";
    else if (_ext == ".php")
        interpreter = "/usr/bin/php-cgi";

    // --- Prepare args ---
    char *argv[3];
    argv[0] = const_cast<char*>(interpreter.c_str());
    argv[1] = const_cast<char*>(_scriptPath.c_str());
    argv[2] = NULL;

    // --- Build environment array ---
    // use current env as base. You can later extend this if you build your own envp
    extern char **environ;

    execve(argv[0], argv, environ);

    // If we reach here, execve failed
    perror("execve");
    _exit(1);
}

CgiState Cgi::handleReadFromCGI() {

    std::cout << "[DEBUG] Handling CGI read for pid = " << _pid << std::endl;
    if (_finished || _outFd < 0)
        return CGI_READY;

    char buf[BUFFER_SIZE_32];
    ssize_t bytesRead = read(_outFd, buf, sizeof(buf));

    if (bytesRead == -1) { //EAGAIN/EWOULDBLOCK
        return CGI_CONTINUE;
    }

    if (bytesRead > 0) {
        if (_resonseData.size() + bytesRead > MAX_CGI_OUTPUT) { //buffer size limits check. how about 100Gb script?
            std::cerr << "[CGI] Output exceeds limit" << std::endl;
            terminate();
            closeOutFd();
            return CGI_ERROR;
        }
        _resonseData.append(buf, bytesRead);
        return CGI_CONTINUE;
    }

    // --- EOF: child finished writing ---
    if (bytesRead == 0) {
        std::cout << "[CGI] EOF reached for outFd = " << _outFd << std::endl;
        std::cout << "[DEBUG] SGI responseData: " << _resonseData << std::endl;

        _finished = true;
        closeOutFd();
        terminate();
        return CGI_READY;
    }
}
CgiState Cgi::handleWriteToCGI() {

    // POST → write body to stdin of CGI
    if (_request.getBody().size() == _bytesWrittenToCgi){
        return CGI_READY;
    }

    const char* body = _request.getBody().c_str() + _bytesWrittenToCgi;
    size_t bytesLeft = _request.getBody().size() - _bytesWrittenToCgi;
    size_t bytesToWrite = std::min(bytesLeft, static_cast<size_t>(BUFFER_SIZE_32));
    ssize_t bytesWritten = write(_inFd, body, bytesToWrite);

    if (bytesWritten < 0) { // the only error could be curnell buffer is full, so we do real error checks after POLL
        return CGI_CONTINUE;
    }
    if (bytesWritten > 0){
        _bytesWrittenToCgi += bytesWritten;
        std::cout << "[CGI] Sending POST body to script: " << _scriptPath;
        std::cout << "[CGI] wrote " << bytesWritten << " bytes to CGI stdin\n";
        return CGI_CONTINUE;
    }
}

void Cgi::terminate() {
    kill(_pid, SIGKILL);
    waitpid(_pid, NULL, WNOHANG);
    _eventLoop.addKilledPid(_pid);
}
void Cgi::cleanup() {
    closeInFd();
    closeOutFd();
}
void Cgi::closeInFd() {
    if(_inFd >= 0) close(_inFd);
    _inFd = -1;
}
void Cgi::closeOutFd() {
    if(_outFd >= 0) close(_outFd);
    _outFd = -1;
}

int Cgi::getInFd() const { return _inFd; }
int Cgi::getOutFd() const { return _outFd; }
int Cgi::getPid() const { return _pid; }
int Cgi::getClientFd() const { return _connection.getFd(); };

time_t Cgi::getStartTime() const { return _startTime; }
const HttpRequest& Cgi::getRequest() const { return _request; }
const std::string& Cgi::getResponseData() const { return _resonseData; }
size_t Cgi::getBytesWrittenToCgi() const {return _bytesWrittenToCgi; }
bool Cgi::isFinished() const {return _finished;}
