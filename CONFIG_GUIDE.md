# WebServ Configuration Guide

## Table of Contents
1. [Introduction](#introduction)
2. [Basic Syntax](#basic-syntax)
3. [Configuration Structure](#configuration-structure)
4. [Server Directives](#server-directives)
5. [Location Directives](#location-directives)
6. [Complete Examples](#complete-examples)
7. [Common Patterns](#common-patterns)
8. [Troubleshooting](#troubleshooting)

---

## Introduction

WebServ uses nginx-style configuration syntax. The config file defines one or more server blocks, each describing how the server should handle HTTP requests on specific ports and domains.

### Key Features
- **Nginx-compliant syntax** - Familiar to anyone who's used nginx
- **Multiple server blocks** - Run multiple virtual hosts
- **Location-based routing** - Different settings for different URL paths
- **CGI support** - Execute Python and PHP scripts
- **File uploads** - Handle file uploads to specific locations
- **Custom error pages** - Branded error responses

---

## Basic Syntax

### Rules
1. **Semicolons required** - All simple directives must end with `;`
2. **Blocks use braces** - `{ }` for server and location blocks
3. **Comments** - Use `#` for comments
4. **Whitespace** - Spaces and newlines are ignored (use for formatting)
5. **Case-sensitive** - Directive names and values are case-sensitive

### Example
```nginx
# This is a comment
server {
    listen 8080;              # Simple directive - semicolon required
    server_name localhost;    # Another simple directive

    location / {              # Block directive - no semicolon
        root /var/www;
        index index.html;
    }                         # Closing brace - no semicolon
}
```

---

## Configuration Structure

```nginx
server {
    # Network configuration
    listen ...;
    server_name ...;

    # Server-wide settings
    root ...;
    index ...;
    error_page ...;

    # Locations (optional)
    location /path {
        # Location-specific settings
    }
}

server {
    # Another server block (optional)
}
```

---

## Server Directives

### Network Configuration

#### `listen`
Defines the address and port the server listens on.

**Syntax:** `listen <address>:<port>;` or `listen <port>;`

**Examples:**
```nginx
listen 8080;                    # Listen on all interfaces, port 8080
listen 127.0.0.1:8080;         # Listen on localhost only
listen 0.0.0.0:8080;           # Explicit: all interfaces
listen 192.168.1.100:80;       # Specific IP address
```

**Multiple listeners:**
```nginx
server {
    listen 0.0.0.0:8080;       # IPv4
    listen 127.0.0.1:8081;     # Also on 8081
}
```

**Required:** Yes (at least one)
**Multiple allowed:** Yes
**Context:** Server only

---

#### `server_name`
Defines server names (domain names) this server block responds to.

**Syntax:** `server_name <name> [<name> ...];`

**Examples:**
```nginx
server_name localhost;
server_name example.com;
server_name example.com www.example.com;
server_name _;  # Default catch-all server
```

**Required:** No
**Multiple allowed:** Yes
**Context:** Server only

---

#### `backlog`
Maximum number of pending connections in the listen queue.

**Syntax:** `backlog <number>;`

**Range:** 1 to 1024

**Example:**
```nginx
backlog 512;  # Recommended for production
backlog 128;  # Smaller for testing
```

**Required:** Yes
**Default:** None
**Context:** Server only

---

### File Serving

#### `root`
Document root directory for serving files.

**Syntax:** `root <path>;`

**Examples:**
```nginx
root /var/www/html/;
root runtime/www/;
root /home/user/public_html/;
```

**Notes:**
- Path can be absolute or relative
- Trailing slash recommended but not required
- Location blocks can override server root

**Required:** Yes (at server level)
**Context:** Server, Location

---

#### `index`
Default file to serve when a directory is requested.

**Syntax:** `index <filename>;`

**Examples:**
```nginx
index index.html;
index home.html;
index default.htm;
```

**Required:** No (but recommended)
**Context:** Server, Location

---

#### `autoindex`
Enable/disable automatic directory listing.

**Syntax:** `autoindex <on|off|true|false|1|0>;`

**Examples:**
```nginx
autoindex on;    # Show directory contents
autoindex off;   # Return 404 if no index file
autoindex true;  # Same as 'on'
autoindex 0;     # Same as 'off'
```

**Required:** No
**Default:** off
**Context:** Server, Location

---

### HTTP Behavior

#### `allow_methods`
Allowed HTTP methods for this server/location.

**Syntax:** `allow_methods <method> [<method> ...];`

**Supported methods:** `GET`, `POST`, `DELETE`

**Examples:**
```nginx
allow_methods GET;                    # Read-only
allow_methods GET POST;               # Read and create
allow_methods GET POST DELETE;        # Full CRUD
```

**Required:** No
**Default:** GET
**Context:** Server, Location

---

#### `client_max_body_size`
Maximum allowed size of client request body (in bytes).

**Syntax:** `client_max_body_size <bytes>;`

**Examples:**
```nginx
client_max_body_size 1048576;      # 1 MB
client_max_body_size 10485760;     # 10 MB
client_max_body_size 52428800;     # 50 MB
client_max_body_size 1073741824;   # 1 GB (max allowed)
```

**Common sizes:**
- 1 KB = 1024
- 1 MB = 1048576
- 10 MB = 10485760
- 100 MB = 104857600

**Required:** Yes
**Max:** 1 GB (1073741824 bytes)
**Context:** Server, Location

---

#### `error_page`
Custom error pages for specific HTTP status codes.

**Syntax:** `error_page <code> [<code> ...] <path>;`

**Examples:**
```nginx
error_page 404 /errors/404.html;
error_page 500 502 503 /errors/50x.html;
error_page 403 /errors/forbidden.html;
error_page 400 401 403 /errors/40x.html;
```

**Common status codes:**
- 400: Bad Request
- 403: Forbidden
- 404: Not Found
- 405: Method Not Allowed
- 413: Payload Too Large
- 500: Internal Server Error
- 502: Bad Gateway
- 503: Service Unavailable

**Required:** No
**Multiple allowed:** Yes
**Context:** Server, Location

---

### Keep-Alive Settings

#### `keepalive_timeout`
Timeout for keep-alive connections (in seconds).

**Syntax:** `keepalive_timeout <seconds>;`

**Examples:**
```nginx
keepalive_timeout 15;   # 15 seconds
keepalive_timeout 30;   # 30 seconds (recommended)
keepalive_timeout 60;   # 1 minute
```

**Required:** No
**Default:** 15
**Range:** 0-3600
**Context:** Server only

---

#### `keepalive_max_requests`
Maximum requests per keep-alive connection.

**Syntax:** `keepalive_max_requests <number>;`

**Examples:**
```nginx
keepalive_max_requests 100;   # Default
keepalive_max_requests 1000;  # High traffic
keepalive_max_requests 50;    # Conservative
```

**Required:** No
**Default:** 100
**Range:** 1-10000
**Context:** Server only

---

### CGI Configuration

#### `cgi_path`
Path(s) to CGI interpreter directories.

**Syntax:** `cgi_path <path> [<path> ...];`

**Examples:**
```nginx
cgi_path /usr/bin/;
cgi_path runtime/www/cgi-bin/;
cgi_path /usr/local/bin/ /usr/bin/;
```

**Required:** No (only if using CGI)
**Context:** Server, Location

---

#### `cgi_ext`
File extensions that should be treated as CGI scripts.

**Syntax:** `cgi_ext <extension> [<extension> ...];`

**Supported extensions:** `.py` (Python), `.php` (PHP)

**Examples:**
```nginx
cgi_ext .py;              # Python only
cgi_ext .php;             # PHP only
cgi_ext .py .php;         # Both Python and PHP
```

**Required:** No (only if using CGI)
**Context:** Server, Location

---

## Location Directives

Location blocks define settings for specific URL paths.

### Syntax
```nginx
location <path> {
    # Directives
}
```

### Matching Rules
Locations are matched by **longest prefix**:

```nginx
location / {
    # Matches: /, /index.html, /api/users, etc.
    # (matches everything)
}

location /api {
    # Matches: /api, /api/users, /api/posts
    # (more specific than /)
}

location /api/admin {
    # Matches: /api/admin, /api/admin/users
    # (most specific)
}
```

**Request:** `GET /api/users`
**Match:** `/api` (longest matching prefix)

---

### Location-Specific Directives

#### `upload_enabled`
Enable file uploads in this location.

**Syntax:** `upload_enabled <on|off|true|false|1|0>;`

**Example:**
```nginx
location /uploads {
    upload_enabled on;
    upload_store runtime/uploads/;
}
```

**Required:** No
**Default:** off
**Context:** Location only

---

#### `upload_store`
Directory where uploaded files are stored.

**Syntax:** `upload_store <path>;`

**Example:**
```nginx
location /uploads {
    upload_enabled on;
    upload_store /var/www/uploads/;
    allow_methods POST DELETE;
}
```

**Required:** Yes (if upload_enabled is on)
**Context:** Location only

---

#### `redirect`
HTTP redirect to another URL.

**Syntax:** `redirect <code> <url>;`

**Codes:** 301 (Permanent), 302 (Temporary), 303 (See Other)

**Examples:**
```nginx
location /old-page {
    redirect 301 /new-page;
}

location /external {
    redirect 302 https://example.com;
}
```

**Required:** No
**Context:** Location only

---

## Complete Examples

### Example 1: Basic Web Server
Simple static file server with error pages.

```nginx
server {
    listen 0.0.0.0:8080;
    server_name localhost;

    backlog 512;
    keepalive_timeout 30;
    keepalive_max_requests 100;

    root /var/www/html/;
    index index.html;
    autoindex off;

    client_max_body_size 10485760;  # 10 MB
    allow_methods GET POST;

    error_page 404 /errors/404.html;
    error_page 500 502 503 /errors/50x.html;

    location / {
        root /var/www/html/;
        index index.html;
        autoindex off;
        allow_methods GET;
    }
}
```

---

### Example 2: Multiple Virtual Hosts
Two different websites on different ports.

```nginx
# Main website
server {
    listen 0.0.0.0:80;
    server_name example.com www.example.com;

    backlog 512;
    root /var/www/example.com/;
    index index.html;

    client_max_body_size 5242880;  # 5 MB
    allow_methods GET POST;

    error_page 404 /404.html;

    location / {
        root /var/www/example.com/;
        index index.html;
        allow_methods GET;
    }
}

# Admin panel
server {
    listen 127.0.0.1:8081;
    server_name admin.example.com;

    backlog 128;
    root /var/www/admin/;
    index admin.html;

    client_max_body_size 1048576;  # 1 MB
    allow_methods GET POST DELETE;

    location / {
        root /var/www/admin/;
        index admin.html;
        allow_methods GET POST;
    }
}
```

---

### Example 3: File Upload Server
Server with file upload capability.

```nginx
server {
    listen 0.0.0.0:8080;
    server_name files.example.com;

    backlog 256;
    root /var/www/files/;
    index index.html;

    client_max_body_size 104857600;  # 100 MB
    allow_methods GET POST DELETE;

    # Main page
    location / {
        root /var/www/files/;
        index index.html;
        allow_methods GET;
        client_max_body_size 1048576;  # 1 MB
    }

    # Upload endpoint
    location /uploads {
        root /var/www/files/uploads/;
        autoindex on;
        allow_methods GET POST DELETE;
        client_max_body_size 104857600;  # 100 MB
        upload_enabled on;
        upload_store /var/www/files/uploads/;
    }
}
```

---

### Example 4: CGI Application Server
Server with Python and PHP CGI support.

```nginx
server {
    listen 0.0.0.0:8080;
    server_name app.example.com;

    backlog 512;
    root /var/www/app/;
    index index.html;

    client_max_body_size 10485760;  # 10 MB
    allow_methods GET POST;

    cgi_path /usr/bin/;
    cgi_ext .py .php;

    # Static files
    location / {
        root /var/www/app/;
        index index.html;
        autoindex off;
        allow_methods GET;
    }

    # CGI scripts
    location /cgi-bin {
        root /var/www/app/cgi-bin/;
        autoindex on;
        allow_methods GET POST;
        client_max_body_size 5242880;  # 5 MB
        cgi_path /usr/bin/;
        cgi_ext .py .php;
    }

    # API endpoint (Python)
    location /api {
        root /var/www/app/api/;
        allow_methods GET POST DELETE;
        cgi_path /usr/bin/;
        cgi_ext .py;
    }
}
```

---

### Example 5: Complete Production Setup
Full-featured production configuration.

```nginx
server {
    # Network
    listen 0.0.0.0:8080;
    listen 127.0.0.1:8081;
    server_name example.com www.example.com;

    # Connection settings
    backlog 1024;
    keepalive_timeout 60;
    keepalive_max_requests 1000;

    # Defaults
    root /var/www/production/;
    index index.html;
    autoindex off;

    client_max_body_size 52428800;  # 50 MB
    allow_methods GET POST DELETE;

    # Error pages
    error_page 400 /errors/400.html;
    error_page 403 /errors/403.html;
    error_page 404 /errors/404.html;
    error_page 405 /errors/405.html;
    error_page 413 /errors/413.html;
    error_page 500 502 503 /errors/50x.html;

    # CGI
    cgi_path /usr/bin/;
    cgi_ext .py .php;

    # Static content
    location / {
        root /var/www/production/public/;
        index index.html;
        autoindex off;
        allow_methods GET;
        client_max_body_size 1048576;  # 1 MB
    }

    # API
    location /api {
        root /var/www/production/api/;
        allow_methods GET POST DELETE;
        client_max_body_size 10485760;  # 10 MB
        cgi_path /usr/bin/;
        cgi_ext .py;
    }

    # File uploads
    location /uploads {
        root /var/www/production/uploads/;
        autoindex on;
        allow_methods GET POST DELETE;
        client_max_body_size 52428800;  # 50 MB
        upload_enabled on;
        upload_store /var/www/production/uploads/;
    }

    # Admin panel
    location /admin {
        root /var/www/production/admin/;
        index admin.html;
        allow_methods GET POST;
        client_max_body_size 5242880;  # 5 MB
    }

    # Redirects
    location /old-site {
        redirect 301 /;
    }
}
```

---

## Common Patterns

### Pattern 1: Inheritance and Overrides
Location blocks inherit server settings but can override them.

```nginx
server {
    # Server defaults
    root /var/www/;
    index index.html;
    allow_methods GET POST DELETE;
    client_max_body_size 10485760;  # 10 MB

    location / {
        # Inherits all server settings
    }

    location /readonly {
        # Override: only GET allowed
        allow_methods GET;
    }

    location /uploads {
        # Override: larger body size
        client_max_body_size 104857600;  # 100 MB
    }
}
```

---

### Pattern 2: Progressive Enhancement
Start simple, add features as needed.

```nginx
# Step 1: Basic server
server {
    listen 8080;
    server_name localhost;
    backlog 128;
    root /var/www/;
    index index.html;
    client_max_body_size 1048576;
    allow_methods GET;
}

# Step 2: Add locations
server {
    listen 8080;
    server_name localhost;
    backlog 128;
    root /var/www/;
    index index.html;
    client_max_body_size 1048576;
    allow_methods GET;

    location / {
        root /var/www/public/;
        allow_methods GET;
    }

    location /admin {
        root /var/www/admin/;
        allow_methods GET POST;
    }
}

# Step 3: Add features (CGI, uploads, etc.)
```

---

### Pattern 3: Security Layers
Different security for different paths.

```nginx
server {
    listen 8080;
    server_name secure.example.com;

    backlog 512;
    root /var/www/secure/;
    client_max_body_size 1048576;  # 1 MB default

    # Public - read-only
    location / {
        allow_methods GET;
        client_max_body_size 1048576;
    }

    # Authenticated users - read/write
    location /user {
        allow_methods GET POST;
        client_max_body_size 5242880;  # 5 MB
    }

    # Admin - full access
    location /admin {
        allow_methods GET POST DELETE;
        client_max_body_size 10485760;  # 10 MB
    }
}
```

---

### Pattern 4: Service Isolation
Separate locations for different services.

```nginx
server {
    listen 8080;
    server_name api.example.com;

    backlog 512;
    root /var/www/api/;
    client_max_body_size 10485760;

    # REST API
    location /api/v1 {
        root /var/www/api/v1/;
        allow_methods GET POST DELETE;
        cgi_path /usr/bin/;
        cgi_ext .py;
    }

    # GraphQL API
    location /graphql {
        root /var/www/api/graphql/;
        allow_methods POST;
        cgi_path /usr/bin/;
        cgi_ext .py;
    }

    # File storage
    location /storage {
        root /var/www/api/storage/;
        allow_methods GET POST DELETE;
        upload_enabled on;
        upload_store /var/www/api/storage/;
        client_max_body_size 52428800;  # 50 MB
    }
}
```

---

## Troubleshooting

### Syntax Errors

**Error:** `Expected ';' after directive`
```nginx
# Wrong
listen 8080

# Correct
listen 8080;
```

---

**Error:** `Expected '{' to start block`
```nginx
# Wrong
server
    listen 8080;
}

# Correct
server {
    listen 8080;
}
```

---

**Error:** `Unexpected token`
```nginx
# Wrong - semicolon after brace
server {
    listen 8080;
};  # Remove this semicolon

# Correct
server {
    listen 8080;
}
```

---

### Validation Errors

**Error:** `Directive 'X' is not allowed in Y context`
```nginx
# Wrong - upload_enabled in server block
server {
    upload_enabled on;  # ERROR: location-only directive
}

# Correct - upload_enabled in location block
server {
    location /uploads {
        upload_enabled on;
    }
}
```

---

**Error:** `Missing required directive: X`
```nginx
# Wrong - missing required directives
server {
    listen 8080;
    # Missing: root, backlog, client_max_body_size
}

# Correct - all required directives present
server {
    listen 8080;
    backlog 512;
    root /var/www/;
    client_max_body_size 1048576;
}
```

---

**Error:** `Duplicate directive: X`
```nginx
# Wrong - duplicate root
server {
    root /var/www/;
    root /var/html/;  # ERROR: duplicate
}

# Correct - only one root at server level
server {
    root /var/www/;

    location /other {
        root /var/html/;  # OK: different context
    }
}
```

---

### Runtime Errors

**Error:** `Failed to open config file`
- Check file path is correct
- Check file permissions (readable)
- Use absolute path or correct relative path

---

**Error:** `Invalid or inaccessible root path`
- Ensure directory exists
- Check directory permissions (readable + executable)
- Use absolute paths for clarity

---

**Error:** `Invalid or inaccessible index file`
- Ensure index file exists in root directory
- Check file permissions (readable)
- Or enable `autoindex on` to skip index requirement

---

**Error:** `Invalid or inaccessible error_page file`
- Error page paths are relative to server `root`
- Check files exist: `<root><error_page_path>`
- Example: root=/var/www/, error_page=/errors/404.html → /var/www/errors/404.html

---

## Quick Reference

### Minimal Working Config
```nginx
server {
    listen 8080;
    backlog 128;
    root /var/www/;
    client_max_body_size 1048576;
}
```

### Common Byte Sizes
```
1 KB   = 1024
10 KB  = 10240
100 KB = 102400
1 MB   = 1048576
10 MB  = 10485760
50 MB  = 52428800
100 MB = 104857600
1 GB   = 1073741824  (maximum)
```

### Boolean Values
All accept: `on`, `off`, `true`, `false`, `1`, `0`

---

## Testing Your Configuration

### 1. Syntax Check
The parser will immediately report syntax errors:
```bash
./webserv my_config.conf
# If syntax error: shows line:column and error message
# If syntax OK: server starts
```

### 2. Validation Check
Parser validates directives and values:
- Unknown directives
- Wrong contexts
- Missing required directives
- Invalid values

### 3. Runtime Check
Test actual functionality:
```bash
# Start server
./webserv my_config.conf

# Test with curl
curl http://localhost:8080/
curl -X POST http://localhost:8080/api
curl -X DELETE http://localhost:8080/file.txt
```

---

## Best Practices

1. **Start simple** - Get basic config working first, then add features
2. **Use comments** - Document non-obvious settings
3. **Consistent formatting** - Use indentation and spacing
4. **Security first** - Restrict methods and body sizes appropriately
5. **Test incrementally** - Test after each major change
6. **Use absolute paths** - Easier to debug than relative paths
7. **Plan locations** - Design URL structure before implementing
8. **Monitor logs** - Use `--log-level debug` during development

---

## Example Workflow

### 1. Create Basic Config
```nginx
server {
    listen 8080;
    server_name localhost;
    backlog 128;
    root /var/www/;
    index index.html;
    client_max_body_size 1048576;
    allow_methods GET;
}
```

### 2. Test
```bash
./webserv test.conf --log-level debug
curl http://localhost:8080/
```

### 3. Add Features
```nginx
server {
    listen 8080;
    server_name localhost;
    backlog 128;
    root /var/www/;
    index index.html;
    client_max_body_size 10485760;
    allow_methods GET POST DELETE;

    error_page 404 /errors/404.html;

    location / {
        allow_methods GET;
    }

    location /api {
        allow_methods GET POST;
        cgi_path /usr/bin/;
        cgi_ext .py;
    }
}
```

### 4. Test Again
```bash
./webserv test.conf --log-level info
curl http://localhost:8080/
curl http://localhost:8080/api/endpoint.py
```

### 5. Iterate
Continue adding features and testing until complete.

---

**Happy configuring!** 🚀
