#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/html\r")
print("\r")
print("<html>")
print("<body>")
print("<h1>Hello from Python CGI!</h1>")
print("<p>Request Method: " + os.environ.get('REQUEST_METHOD', 'N/A') + "</p>")
print("<p>Query String: " + os.environ.get('QUERY_STRING', 'N/A') + "</p>")

# Read POST body if present
content_length = os.environ.get('CONTENT_LENGTH')
if content_length and int(content_length) > 0:
    body = sys.stdin.read(int(content_length))
    print("<p>POST Body: " + body + "</p>")

print("</body>")
print("</html>")
