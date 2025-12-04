/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: antonsplavnik <antonsplavnik@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/27 13:07:59 by antonsplavn       #+#    #+#             */
/*   Updated: 2025/12/03 21:36:52 by antonsplavn      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "cgi.hpp"
#include "connection.hpp"
#include "event_loop.hpp"
#include "response.hpp"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <algorithm>
#include <signal.h>
#include <sys/resource.h>

#define MAX_CGI_OUTPUT (10 * 1024 * 1024)

Cgi::Cgi(EventLoop& eventLoop, const HttpRequest& request, int connectionFd)
        : _pid(-1),
          _inFd(-1),
          _outFd(-1),
          _finished(false),
          _startTime(0),
          _bytesWrittenToCgi(0),
          _connectionFd(connectionFd),
          _request(request),
          _eventLoop(eventLoop) {}

Cgi::Cgi(const Cgi& other)
        : _pid(other._pid),
          _inFd(other._inFd),
          _outFd(other._outFd),
          _finished(other._finished),
          _startTime(other._startTime),
          _bytesWrittenToCgi(other._bytesWrittenToCgi),
          _connectionFd(other._connectionFd),
          _responseData(other._responseData),
          _request(other._request),
          _eventLoop(other._eventLoop) {
	// Transfer ownership - prevent source from closing FDs (const_cast - removes const from the passed object)
	const_cast<Cgi&>(other)._inFd = -1;
	const_cast<Cgi&>(other)._outFd = -1;
	const_cast<Cgi&>(other)._pid = -1;
}

Cgi::~Cgi() {
    cleanup();
}

bool Cgi::start(const Connection& connection) {

    int inpipe[2];
    int outpipe[2];

    if (pipe(inpipe) < 0 || pipe(outpipe) < 0) {
        perror("pipe");
        return false;
    }

	if (access(connection.getRoutingResult().mappedPath.c_str(), X_OK) != 0) {
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
            getrlimit(RLIMIT_AS, &mem_limit);
            mem_limit.rlim_cur = 50 * 1024 * 1024;

            #ifndef __APPLE__
                if (setrlimit(RLIMIT_AS, &mem_limit) != 0) {
                    perror("setrlimit");
                    _exit(126);
                }
            #endif
        }

        if (!chdirToScriptDir(connection.getRoutingResult().mappedPath)) {
            _exit(126);  // non-zero => parent will treat as 500
        }

        // prepEnv(_request, _scriptPath);  // Ensure PATH is set
        executeCGI(connection);
    }

    // --- PARENT ---

    _startTime = time(NULL);

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

void Cgi::executeCGI(const Connection& connection) {

    std::string ext = connection.getRoutingResult().cgiExtension;

    // --- Determine interpreter ---
    std::string interpreter;
    if (ext == ".py")
        interpreter = findInterpreter("python3");
    else if (ext == ".php")
        interpreter = findInterpreter("php-cgi");

    if (interpreter.empty()) {
        std::cerr << "[CGI] Interpreter not found for extension: " << ext << std::endl;
        _exit(127);
    }

    // --- Prepare args ---
    char *argv[3];
    argv[0] = const_cast<char*>(interpreter.c_str());
    argv[1] = const_cast<char*>(connection.getRoutingResult().mappedPath.c_str());
    argv[2] = NULL;

    // --- Build environment array ---
    char** envp = prepEnvVariables(connection);

    execve(argv[0], argv, envp);

    // If we reach here, execve failed
    perror("execve");
    _exit(1);
}
std::string Cgi::findInterpreter(const std::string& name) {
    const char* searchPaths[] = {
        "/usr/bin/",
        "/usr/local/bin/",
        "/opt/homebrew/bin/",
        NULL
    };

    for (int i = 0; searchPaths[i] != NULL; i++) {
        std::string fullPath = std::string(searchPaths[i]) + name;
        if (access(fullPath.c_str(), X_OK) == 0) {
            return fullPath;
        }
    }
    return "";
}
char** Cgi::prepEnvVariables(const Connection& connection) {

    std::vector<std::string> cgiHeaders =  _request.getCgiHeadersString();

    int staticVars = 19;
    int headersCount = cgiHeaders.size(); // Dynamic HTTP_* headers
    int totalSize = staticVars + headersCount + 1;  // +1 for NULL terminator

    char** envp = new char*[totalSize];
    int i = 0;
    std::string buffer;


    /////// MANDATORY //////////

    // REQUEST HEADERS
    for (size_t j = 0; j < cgiHeaders.size(); j++)
    {
        envp[i++] = strdup(cgiHeaders[j].c_str());
    }

    // GATEWAY_INTERFACE
    envp[i++] = strdup("GATEWAY_INTERFACE=CGI/1.1");

    // SERVER_PROTOCOL
    buffer = "SERVER_PROTOCOL=" + _request.getProtocolVersion();  // request
    envp[i++] = strdup(buffer.c_str());

    // SERVER_NAME
    std::string serverName = _request.getHost();
     // Strip port if present
    size_t colon = serverName.find(":");
    if(colon != std::string::npos)
        serverName = serverName.substr(0, colon);
    // Fallback for HTTP/1.0 if empty
    if(serverName.empty())
        serverName =  connection.getRoutingResult().serverConfig->server_names[0];
    buffer = "SERVER_NAME=" + serverName;  // request
    envp[i++] = strdup(buffer.c_str());

    // SERVER_PORT
    std::ostringstream portOss;
    portOss << connection.getServerPort();
    buffer = "SERVER_PORT=" + portOss.str(); // serverPort (connection)
    envp[i++] = strdup(buffer.c_str());

    // REQUEST_METHOD
    buffer = "REQUEST_METHOD=" + _request.getMethod(); // request
    envp[i++] = strdup(buffer.c_str());

    // QUERY_STRING
    buffer = std::string("QUERY_STRING=") + _request.getQuery(); // request
    envp[i++] = strdup(buffer.c_str());

    // SCRIPT_NAME
    buffer = "SCRIPT_NAME=" + connection.getRoutingResult().scriptName;
    envp[i++] = strdup(buffer.c_str());

    // PATH_INFO
    buffer = "PATH_INFO=" + connection.getRoutingResult().pathInfo;
    envp[i++] = strdup(buffer.c_str());

    // PATH_TRANSLATED
    buffer = "PATH_TRANSLATED=" + connection.getRoutingResult().pathTranslated;
    envp[i++] = strdup(buffer.c_str());

    // CONTENT_LENGTH
    std::ostringstream oss;
    oss << _request.getContentLength();
    buffer = "CONTENT_LENGTH=" + oss.str(); // request
    envp[i++] = strdup(buffer.c_str());

    // CONTENT_TYPE
    buffer = "CONTENT_TYPE=" + _request.getContentType(); // request
    envp[i++] = strdup(buffer.c_str());

    // REMOTE_ADDR
    buffer = "REMOTE_ADDR=" + connection.getIp(); // connection
    envp[i++] = strdup(buffer.c_str());

    // REMOTE_PORT
    std::ostringstream remotePortOss;
    remotePortOss << connection.getConnectionPort();
    buffer = "REMOTE_PORT=" + remotePortOss.str(); // connection
    envp[i++] = strdup(buffer.c_str());


    /////// PHP SPECIFIC //////////

    //REDIRECT_STATUS
    buffer = "REDIRECT_STATUS=200";
    envp[i++] = strdup(buffer.c_str());

    // SCRIPT_FILENAME
    buffer = "SCRIPT_FILENAME=" + connection.getRoutingResult().mappedPath;
    envp[i++] = strdup(buffer.c_str());


    /////// EXTRA //////////

    // DOCUMENT_ROOT
    buffer = "DOCUMENT_ROOT=" + connection.getRoutingResult().location->root;
    envp[i++] = strdup(buffer.c_str());

    // REQUEST_URI
    buffer = "REQUEST_URI=" + _request.getUri();
    envp[i++] = strdup(buffer.c_str());

    // PATH
    // envp[i++] = strdup("PATH=/usr/local/bin:/usr/bin:/bin");
    envp[i++] = strdup("PATH=/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin");

    // SERVER_SOFTWARE
    envp[i++] = strdup("SERVER_SOFTWARE=WebServ/1.0");

    envp[i++] = NULL;

    return envp;
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
    else if (bytesRead == 0) { // --- EOF: child finished writing ---
        std::cout << "[CGI] EOF reached for outFd = " << _outFd << std::endl;
        std::cout << "[DEBUG] CGI responseData: " << _responseData << std::endl;

        _finished = true;
        closeOutFd();
        terminate();
        return CGI_READY;
    }
    else { // bytesRead > 0
        if (_responseData.size() + bytesRead > MAX_CGI_OUTPUT) { //buffer size limits check. how about 100Gb script?
            std::cerr << "[CGI] Output exceeds limit" << std::endl;
            closeOutFd();
            terminate();
            return CGI_ERROR;
        }
        _responseData.append(buf, bytesRead);
        return CGI_CONTINUE;
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
    else if (bytesWritten > 0){
        _bytesWrittenToCgi += bytesWritten;
        /* std::cout << "[CGI] Sending POST body to script: " << _scriptPath; */
        std::cout << "[CGI] wrote " << bytesWritten << " bytes to CGI stdin\n";
        return CGI_CONTINUE;
    }
    else { // bytesWritten == 0 (shouldn't happen, but handle for completeness)
        return CGI_CONTINUE;
    }
}

void Cgi::terminate() {
    if (_pid > 0) {
        kill(_pid, SIGKILL);
        int result = waitpid(_pid, NULL, WNOHANG);
        if (result == 0) _eventLoop.addKilledPid(_pid);
        _pid = -1;
    }
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
