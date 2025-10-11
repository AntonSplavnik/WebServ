//
// Created by Maksim Kokorev on 18.09.25.
//

#ifndef HTTP_HPP
#define HTTP_HPP

// Structure that holds parsed information from the HTTP request.
// This is the canonical format our server will work with.
// Each field has an explanation + example based on this request:
//
//   POST /cgi-bin/hello.py/extra/data?name=world&age=22 HTTP/1.1
//   Host: localhost:8080
//   Content-Type: application/x-www-form-urlencoded
//   Content-Length: 13
//
//   password=1234
//
#include <string>
#include <map>

struct UrlStruct {
    // HTTP method (the action the client wants).
    // Example: "GET", "POST", "PUT", "DELETE"
    // In request: "POST"
    std::string method;

    // The full URL as received in the request line (without the host).
    // Example: "/cgi-bin/hello.py/extra/data?name=world&age=22"
    std::string full_url;

    // The actual CGI script path part (maps to an executable script).
    // Example: "/cgi-bin/hello.py"
    std::string script_name;

    // Extra path information that comes after the script.
    // CGI spec calls this PATH_INFO.
    // Example: "/extra/data"
    std::string path_info;

    // The query string, the part after '?' in the URL.
    // Example: "name=world&age=22"
    std::string query_string;

    // HTTP version string from the request line.
    // Example: "HTTP/1.1"
    std::string version;

    // All headers from the request as key-value pairs.
    // Example:
    //   "Host" -> "localhost:8080"
    //   "Content-Type" -> "application/x-www-form-urlencoded"
    //   "Content-Length" -> "13"
    std::map<std::string, std::string> headers;

    // Body of the request (present in POST, PUT, etc.).
    // Example: "password=1234"
    std::string body;

    // -------------------
    // Derived / Convenience Fields
    // -------------------

    // Host header value (may include port).
    // Example: "localhost:8080"
    std::string host;

    // Shortcut to Content-Type header.
    // Example: "application/x-www-form-urlencoded"
    std::string content_type;

    // Content length parsed as integer.
    // Example: 13
    size_t content_length = 0;

    // Server name (hostname only, without port).
    // Example: "localhost"
    std::string server_name;

    // Server port parsed from Host header or socket.
    // Example: 8080
    int server_port = 0;

    // Whether this request should be executed as CGI.
    // Example: true (if script_name starts with "/cgi-bin/")
    bool is_cgi = false;
};


#endif //HTTP_HPP
