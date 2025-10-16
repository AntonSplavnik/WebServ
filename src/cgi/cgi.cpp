#include "cgi.hpp"
#include "../http_response/http_response.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <poll.h>


Cgi::Cgi(const std::string &path,
         const HttpRequest &req,
         std::map<int, ClientInfo> &clients,
         int clientFd)
    : pid(-1),
      outFd(-1),
      finished(false),
      inFd(-1),
      scriptPath(path),
      request(req),
      _clients(clients),
      _clientFd(clientFd),
      startTime(time(NULL)) {}


Cgi::~Cgi() {
    if (outFd >= 0) close(outFd);
    if (inFd >= 0) close(inFd);
}

void Cgi::setEnv(const HttpRequest &request, const std::string &scriptPath) {
    setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
    setenv("SCRIPT_FILENAME", scriptPath.c_str(), 1);
    setenv("REQUEST_METHOD", request.getMethod().c_str(), 1);
    setenv("QUERY_STRING", request.getQueryString().c_str(), 1);


    // Basic headers
    std::string host = "localhost";
    std::map<std::string, std::string> headers = request.getHeaders();
    if (headers.find("host") != headers.end())
        host = headers["host"];
    setenv("SERVER_NAME", host.c_str(), 1);
    setenv("SERVER_PORT", "8080", 1);

    // POST-specific
    if (request.getMethod() == "POST") {
        std::string body = request.getBody();
        std::string contentType = request.getContenType();
        std::string contentLength = std::to_string(body.size());

        setenv("CONTENT_TYPE", contentType.c_str(), 1);
        setenv("CONTENT_LENGTH", contentLength.c_str(), 1);
    }
    // DELETE — просто указываем метод и пустое тело
    else if (request.getMethod() == "DELETE") {
        setenv("CONTENT_LENGTH", "0", 1);
    }
}


bool Cgi::start() {
    int inpipe[2];
    int outpipe[2];
    if (pipe(inpipe) < 0 || pipe(outpipe) < 0) {
        perror("pipe");
        return false;
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


        setEnv(request, scriptPath);  // Ensure PATH is set

        // ✅ use env to locate python3 dynamically
        execl("/usr/bin/env", "env", "python3", scriptPath.c_str(), NULL);
        // Alternative (direct): execl("/usr/bin/python3", "python3", scriptPath.c_str(), NULL);

        perror("execl");
        _exit(1);
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
        if (!body.empty()) {
            ssize_t written = write(inFd, body.c_str(), body.size());
            std::cout << "[CGI] wrote " << written << " bytes to CGI stdin\n";
        }
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
        std::string Path = request.getPath();
        std::string mappedPath = "/Users/tghnx1/Desktop/42/Webserv42/runtime/www" + Path; //TODO: map properly
        std::cout << "mappedPath: " << mappedPath << std::endl;
        HttpResponse response(request);
        response.generateCgiResponse(buffer);
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

