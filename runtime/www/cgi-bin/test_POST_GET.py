#!/usr/bin/env python3
import os
import cgi
import cgitb

cgitb.enable()  # helpful for debugging

print("Content-Type: text/html\r\n\r\n")

print("""
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>CGI POST/GET Debug</title>
<style>
    body { font-family: monospace; background: #1e1e1e; color: #dcdcdc; padding: 20px; }
    h1 { color: #9cdcfe; }
    h2 { color: #ce9178; margin-top: 20px; }
    pre { background: #252526; padding: 10px; border-radius: 8px; }
    table { border-collapse: collapse; }
    td, th { border: 1px solid #444; padding: 5px 10px; }
</style>
</head>
<body>
<h1>🚀 CGI Environment Debug</h1>
""")

# --- Environment variables ---
print("<h2>Environment Variables</h2>")
print("<pre>")
for key in sorted(os.environ.keys()):
    print(f"{key} = {os.environ[key]}")
print("</pre>")

# --- Query string ---
qs = os.environ.get("QUERY_STRING", "")
print("<h2>GET Parameters</h2>")
if qs:
    print("<pre>")
    for pair in qs.split("&"):
        print(pair)
    print("</pre>")
else:
    print("<p><i>No query parameters</i></p>")

# --- POST data ---
form = cgi.FieldStorage()
print("<h2>POST Parameters</h2>")
if form:
    print("<pre>")
    for key in form.keys():
        print(f"{key} = {form.getvalue(key)}")
    print("</pre>")
else:
    print("<p><i>No POST data</i></p>")

print("<hr><center>WebServ42 CGI Test</center></body></html>")
