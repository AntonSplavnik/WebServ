#!/usr/bin/env python3
import os
import html

# Build the HTML body
cwd = os.getcwd()
script_path = os.path.abspath(__file__)
files = os.listdir(".")
listing = "".join(f"<li>{html.escape(f)}</li>" for f in files)

body = f"""<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>CGI chdir test</title></head>
<body>
<h2>=== CGI chdir test ===</h2>
<p><b>Current working directory:</b> {html.escape(cwd)}</p>
<p><b>Script absolute path:</b> {html.escape(script_path)}</p>
<h3>Listing local files:</h3>
<ul>{listing or "<li>No files found</li>"}</ul>
</body>
</html>"""

# Print the headers
print("Status: 200 OK")
print("Content-Type: text/html; charset=utf-8")
print("Content-Length:", len(body))
print("Connection: close")
print("Cache-Control: no-cache, no-store, must-revalidate")
print("Pragma: no-cache")
print("Expires: 0")
print("Set-Cookie: sessionid=abc123; HttpOnly; Path=/")
print("X-Powered-By: Python/3 CGI\n")

# Print the body
print(body)
