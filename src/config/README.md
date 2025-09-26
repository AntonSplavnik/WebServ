# 📝 Config Parser Documentation

## 📌 Overview
Our webserver uses a custom **nginx-like configuration file** to define how it should listen for connections, serve content, and handle routes.  
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
- `server_name <name>`  
- `root <path>`  
- `index <file>`  
- `autoindex on|off`  
- `client_max_body_size <bytes>`  
- `access_log <path>`  
- `error_log <path>`  
- `error_page <code> <path>`  
- `cgi_ext <.ext>`  
- `cgi_path <path>`  
- `backlog <value>`  

### Location-level (`location /path { … }`)
- `root <path>`  
- `index <file>`  
- `autoindex on|off`  
- `allow_methods GET|POST|DELETE`  
- `redirect <code> <url>`  
- `upload_enabled on|off`  
- `upload_store <path>`  
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
