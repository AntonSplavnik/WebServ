#include "cgi.hpp"
#include "../http_response/http_response.hpp"
#include "../logging/logger.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <poll.h>


Cgi::Cgi(const std::string &path,
         const HttpRequest &req,
         std::map<int, ClientInfo> &clients,
         int clientFd, const LocationConfig* loc, std::string cgiExt)
    : pid(-1),
      outFd(-1),
      finished(false),
      _matchedLoc(loc),
      ext(cgiExt),
      inFd(-1),
      scriptPath(path),
      request(req),
      _clients(clients),
      _clientFd(clientFd),
      startTime(time(NULL))
       {}


Cgi::~Cgi() {
    if (outFd >= 0) close(outFd);
    if (inFd >= 0) close(inFd);
}
void Cgi::setEnv(const HttpRequest &request, const std::string &scriptPath) {
    //  Base CGI variables
    setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
    setenv("SERVER_PROTOCOL", request.getVersion().c_str(), 1);
    setenv("SERVER_SOFTWARE", "Webserv42/1.0", 1); //TODO: set somewhere and take it from there

    //  Server name and port
    std::string host = "localhost";
    std::map<std::string, std::string> headers = request.getHeaders();
    std::map<std::string, std::string>::const_iterator it = headers.find("host");
    if (it != headers.end())
        host = it->second;
    setenv("SERVER_NAME", host.c_str(), 1);
    setenv("SERVER_PORT", "8080", 1); // or from _matchedLoc / configData
    //  Request details
    setenv("REQUEST_METHOD", request.getMethod().c_str(), 1);
    setenv("QUERY_STRING", request.getQueryString().c_str(), 1);
    setenv("SCRIPT_FILENAME", scriptPath.c_str(), 1);
    setenv("SCRIPT_NAME", request.getNormalizedReqPath().c_str(), 1);
    setenv("PATH", "/usr/local/bin:/usr/bin:/bin", 1);
	setenv("DOCUMENT_ROOT", _matchedLoc ? _matchedLoc->root.c_str() : ".", 1);
	setenv("REDIRECT_STATUS", "200", 1); // required for php-cgi
	setenv("REQUEST_URI", request.getNormalizedReqPath().c_str(), 1);
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
     	it != headers.end(); ++it)
	{
    	std::string key = "HTTP_" + it->first;
    	std::transform(key.begin(), key.end(), key.begin(), ::toupper);
    	std::replace(key.begin(), key.end(), '-', '_');
    	setenv(key.c_str(), it->second.c_str(), 1);
	}


    //  Content info (for POST, PUT)
    std::string body = request.getBody();
    std::string contentLength = "0";
    if (!body.empty()) {
        std::ostringstream oss;
        oss << body.size();
        contentLength = oss.str();
    }
    setenv("CONTENT_LENGTH", contentLength.c_str(), 1);
    setenv("CONTENT_TYPE", request.getContenType().c_str(), 1);

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
    std::string scriptName = request.getMappedPath();

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


void Cgi::executeCgiWithArgs()
{
    // --- Determine interpreter ---
    std::string interpreter;
    if (ext == ".py")
        interpreter = "/usr/bin/python3";
    else if (ext == ".php")
        interpreter = "/usr/bin/php-cgi";

    // --- Prepare args ---
    char *argv[3];
    argv[0] = const_cast<char*>(interpreter.c_str());
    argv[1] = const_cast<char*>(scriptPath.c_str());
    argv[2] = NULL;

    // --- Build environment array ---
    extern char **environ; // use current env as base
    // You can later extend this if you build your own envp

    execve(argv[0], argv, environ);

    // If we reach here, execve failed
    perror("execve");
    _exit(1);
}


bool Cgi::start() {

  //TODO: unchunk (ask if Damien makes it, so maybe i can use it here)
    int inpipe[2];
    int outpipe[2];
    if (pipe(inpipe) < 0 || pipe(outpipe) < 0) {
        perror("pipe");
        return false;
    }

	if (access(scriptPath.c_str(), X_OK) != 0) {
    	perror("access");
    	return false; // will trigger 500
	}
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return false;
    }

    if (pid == 0) {
        // --- CHILD ---
        dup2(inpipe[0], STDIN_FILENO);
        dup2(outpipe[1], STDOUT_FILENO);
        close(inpipe[1]);
        close(outpipe[0]);

        if (!chdirToScriptDir()) {
            _exit(126);  // non-zero => parent will treat as 500
        }

        setEnv(request, scriptPath);  // Ensure PATH is set
        executeCgiWithArgs();
    }

    // --- PARENT ---
    close(inpipe[0]);
    close(outpipe[1]);
    inFd = inpipe[1];
    outFd = outpipe[0];

    fcntl(outFd, F_SETFL, O_NONBLOCK);
    // If POST → write body to stdin of CGI
    if (request.getMethod() == "POST") {
    	const std::string &body = request.getBody();
    	ssize_t totalWritten = 0;
    	ssize_t n;

    	while (totalWritten < (ssize_t)body.size()) {
        	n = write(inFd, body.c_str() + totalWritten, body.size() - totalWritten);
        	if (n < 0) {
            	if (errno == EINTR) continue;      // retry on interrupt
            	perror("[CGI] write error");
            	break;
        	}
        	totalWritten += n;
    	}
		std::cout << "[CGI] Sending POST body to script: " << scriptPath;

    	std::cout << "[CGI] wrote " << totalWritten << " bytes to CGI stdin\n";
	}

    close(inFd);  // always close stdin, so child gets EOF
    return true;
}


bool Cgi::handleRead() {
  std::cout << "Handling CGI read for pid=" << pid << std::endl;
    if (finished || outFd < 0)
        return false;

    char buf[4096];
    ssize_t n = read(outFd, buf, sizeof(buf));

	if (n == -1) {
		HttpResponse errResp(request);
   	 		errResp.generateResponse(500, false, "");
    		_clients[_clientFd].responseData = errResp.getResponse();
    		_clients[_clientFd].state = SENDING_RESPONSE;
    		return false;
	}
    if (n > 0) {
        buffer.append(buf, n);
        return true;
    }

    if (n == 0) {
        std::cout << "[CGI] EOF reached for outFd=" << outFd << std::endl;
        // --- EOF: child finished writing ---
        finished = true;
        close(outFd);
        outFd = -1;

        int status;
        waitpid(pid, &status, WNOHANG);  // reap zombie safely
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    		HttpResponse errResp(request);
   	 		errResp.generateResponse(500, false, "");
    		_clients[_clientFd].responseData = errResp.getResponse();
    		_clients[_clientFd].state = SENDING_RESPONSE;
    		return false;
		}
        //std::string Path = request.getPath();
        std::string mappedPath = request.getMappedPath();
        std::cout << "mappedPath: " << mappedPath << std::endl;
        HttpResponse response(request);
        response.generateResponse(200, true, buffer);
        _clients[_clientFd].responseData = response.getResponse();
        _clients[_clientFd].state = SENDING_RESPONSE;


        //debug
        std::cout << "responseData: " << _clients[_clientFd].responseData << std::endl;
        return false;
    }
    std::cout << "not a cgi" << std::endl;
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        return true;

    perror("read");
    return false;
}

