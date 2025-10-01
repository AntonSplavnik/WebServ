# 📝 Config Parser Documentation

## 📌 Overview

Our webserver uses a custom **nginx-like configuration file** to define how it should listen for connections, serve
content, and handle routes.  
The **Config Parser** is responsible for:

- Reading the configuration file.
- Validating all directives.
- Applying defaults where necessary.
- Building internal structures (`ConfigData` and `LocationConfig`) for runtime use.

On success, the parser returns a list of fully validated servers (`std::vector<ConfigData>`).


---

## ✅ Supported Directives

### Server-level (`server { … }`)

- `listen <ip:port | hostname:port>`
    - listen tells your web server which IP/hostname + port combination to bind a socket to. Multiple listeners = the
      same server will accept connections on several network endpoints.
- `server_name <name>`
    - Allowed multiple times to specify several names (e.g. example.com, www.example.com).
- `root <path>`
    - Filesystem path used as the document root for serving static files.
- `index <file>`
    - Default file to serve when a directory is requested (e.g. index.html).
- `autoindex on|off`
    - 'The autoindex directive controls whether the server should generate and return a directory listing when a client
      requests a folder path:
      autoindex on; – if no index file (e.g. index.html) is found, the server generates a simple HTML page listing all
      files in the directory.
      autoindex off; (default) – if no index file is present, the server returns 403 Forbidden.
      Note: index files always take precedence. Directory listings are mostly useful for development or for special
      folders (e.g. /downloads/).
- `client_max_body_size <bytes>`
- Maximum allowed size of the client request body (e.g. for POST uploads). Requests exceeding this limit receive a 413
  Payload Too Large response.
- `access_log <path>`
    - Every handled request:
      Typical entries include:
      Client IP address,
      Timestamp,
      HTTP method and request path,
      Response status code (200, 404, 500, …),
      Response size (bytes)
- `error_log <path>`
    - Problems and internal errors:
      Typical entries include: Configuration parsing errors (invalid directive, missing value), Startup errors (failed
      to bind to a port, missing files), Runtime errors (permissions denied, invalid path, CGI failures), Critical
      events (server crashes, unhandled exceptions)
- `error_page <code> <path>`
    - Custom error page for the given HTTP status code (e.g. 404 /errors/40x.html).
- `cgi_ext <.ext>`
    - File extension(s) that should be processed by a CGI interpreter (e.g. .py, .php).
- `cgi_path <path>`
    - Filesystem path to the CGI interpreter executable (e.g. /usr/bin/python3).
- `backlog <value>`
    - Size of the connection queue for the listen socket; higher values allow handling more simultaneous pending
      connections.

### Location-level (`location /path { … }`)

- `root <path>`
- `index <file>`
- `autoindex on|off`
- `allow_methods GET|POST|DELETE`
- `redirect <code> <url>`
    - Allows specifying a redirection for this location. The server responds with the given HTTP status code (e.g. 301,
        302) and the target URL.
- `upload_enabled on|off`
    - Enables or disables file uploads for this location.
- `upload_store <path>`
    - Filesystem path where uploaded files should be saved. Must be an existing directory with write permissions.
        - Case 1: upload_enabled off;
          Uploads are disabled.
          upload_store (if present) is ignored.
          Any request trying to upload a file (e.g. POST with body) will be rejected.
          Case 2: upload_enabled on; but no upload_store
          ❌ Invalid configuration.
          Parser throws an error: “upload_enabled is on but upload_store is not set in location …”
          Case 3: upload_enabled on; + upload_store /path/to/dir;
          ✅ Uploads are enabled for this location
          Incoming file data is written to the specified directory.
          The directory must exist and be writable (W_OK | X_OK).
- `error_page <code> <path>`
- `client_max_body_size <bytes>`
- `cgi_ext <.ext>`
- `cgi_path <path>`

---

## 🗂 Data Structures

### `ConfigData`

Represents one `server { … }` block.  
Holds:

- Listeners (`host:port` pairs)
- Server names
- Root, index, autoindex
- Logs (access/error)
- Error pages
- Allowed methods
- Client body size limit
- CGI config (extensions, path)
- Locations (vector of `LocationConfig`)

### `LocationConfig`

Represents one `location { … }` block.  
Holds:

- Path (must start with `/`)
- Root, index, autoindex
- Allowed methods
- Error pages
- Max body size
- CGI config
- Upload config (enabled + directory)
- Redirect (code + target)

---

## 🔄 Parsing Flow

1. **parseConfig(path)**
    - Opens file.
    - Iterates line by line.
    - Detects `server { … }` and calls `parseServerBlock`.

2. **parseServerBlock**
    - Reads server-level directives.
    - On `location { … }`, calls `parseLocationBlock`.

3. **parseLocationBlock**
    - Reads location directives until `}`.
    - Calls directive parsers:
        - `parseCommonConfigField` (shared)
        - `parseLocationConfigField` (specific)

4. **Validation (`validateConfig`)**
    - Ensures required fields exist.
    - Applies inheritance and defaults.
    - Checks file paths, permissions, duplicates.

---

## ⚠️ Error Handling

- Parser throws `ConfigParseException` with a descriptive message.
- Examples:
    - Unknown directive
    - Duplicate `root` or `index`
    - Invalid paths or files
    - Missing required directives
    - Unmatched braces

---

## 📄 Example Config

```nginx
server {
    listen 127.0.0.1:8080;
    server_name example.com;
    root ./www;
    index index.html;
    autoindex off;

    error_page 404 ./errors/404.html;
    client_max_body_size 3000000;

    access_log ./logs/access.log;
    error_log ./logs/error.log;

    location /upload {
        upload_enabled on;
        upload_store ./www/uploads;
        allow_methods POST;
    }

    location /cgi {
        cgi_ext .py;
        cgi_path ./cgi-bin;
    }

    location /redirect {
        redirect 302 /new-location;
    }
}
```

---

## 🏁 Output

After parsing, the program produces:

```cpp
std::vector<ConfigData> servers = config.getServers();
```

Each `ConfigData` contains one validated server, with all its listeners and locations ready for the runtime HTTP engine.
