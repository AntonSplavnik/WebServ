#!/usr/bin/env python3
import os
import sys
import json
import urllib.parse
import cgi

def html_escape(s):
    return (s or "").replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

def read_post_data():
    """Read POST body from stdin according to CONTENT_LENGTH."""
    try:
        length = int(os.environ.get("CONTENT_LENGTH", 0))
        return sys.stdin.read(length) if length > 0 else ""
    except ValueError:
        return ""

# --- Determine request method ---
method = os.environ.get("REQUEST_METHOD", "GET").upper()
query_string = os.environ.get("QUERY_STRING", "")
content_type = os.environ.get("CONTENT_TYPE", "")

params = {}
raw_body = ""

if method == "GET":
    # GET parameters come from QUERY_STRING
    params = urllib.parse.parse_qs(query_string)
elif method == "POST":
    raw_body = read_post_data()

    if "application/x-www-form-urlencoded" in content_type:
        params = urllib.parse.parse_qs(raw_body)
    elif "multipart/form-data" in content_type:
        form = cgi.FieldStorage()
        for key in form.keys():
            params[key] = [form.getvalue(key)]
    elif "application/json" in content_type:
        try:
            data = json.loads(raw_body)
            params = {k: [str(v)] for k, v in data.items()}  # uniform dict-of-lists
        except Exception as e:
            params = {"error": [f"Invalid JSON: {e}"]}
    else:
        params = {"raw_body": [raw_body]}

# --- Important environment variables to display ---
important_vars = [
    "AUTH_TYPE", "CONTENT_LENGTH", "CONTENT_TYPE", "GATEWAY_INTERFACE",
    "PATH_INFO", "PATH_TRANSLATED", "QUERY_STRING", "REMOTE_ADDR",
    "REMOTE_HOST", "REMOTE_IDENT", "REMOTE_USER", "REQUEST_METHOD",
    "SCRIPT_NAME", "SERVER_NAME", "SERVER_PORT", "SERVER_PROTOCOL",
    "SERVER_SOFTWARE"
]

# --- CGI Header (must come first) ---
print("Content-Type: text/html; charset=utf-8\n")

# --- HTML Output ---
print("<!DOCTYPE html>")
print("<html><head>")
print("<meta charset='utf-8'>")
print("<title>CGI Debug</title>")
print("<style>")
print("""
body { font-family: monospace; background: #1e1e1e; color: #dcdcdc; padding: 20px; }
h1 { color: #9cdcfe; }
h2 { color: #ce9178; margin-top: 20px; }
pre { background: #252526; padding: 10px; border-radius: 8px; }
table { border-collapse: collapse; margin-top: 10px; }
td, th { border: 1px solid #444; padding: 5px 10px; }
th { background: #333; }
""")
print("</style></head><body>")

print("<h1>🚀 CGI Debug</h1>")
print(f"<h2>Method: {method}</h2>")
print(f"<h2>Content-Type: {html_escape(content_type)}</h2>")

# --- Parameters section ---
if params:
    print("<h2>All Parameters</h2><pre>")
    for k, v in params.items():
        print(f"{html_escape(k)} = {html_escape(', '.join(v))}")
    print("</pre>")
else:
    print("<h2>All Parameters</h2><pre>No parameters received</pre>")

# --- JSON representation ---
print("<h2>JSON</h2><pre>")
print(html_escape(json.dumps(params, indent=2)))
print("</pre>")

# --- Raw POST body ---
if raw_body:
    print("<h2>Raw POST Body</h2><pre>")
    print(html_escape(raw_body))
    print("</pre>")

# --- Environment variables ---
print("<h2>Environment Variables</h2>")
print("<table>")
print("<tr><th>Variable</th><th>Value</th></tr>")
for var in important_vars:
    val = os.environ.get(var, "")
    print(f"<tr><td>{html_escape(var)}</td><td>{html_escape(val)}</td></tr>")
print("</table>")

print("<hr><div style='text-align:center'>WebServ42 CGI Test</div>")
print("</body></html>")
