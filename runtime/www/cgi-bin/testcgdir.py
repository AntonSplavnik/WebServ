#!/usr/bin/env python3
import os

print("Status: 200 OK")
print("Content-Type: text/html; charset=utf-8")
print("Content-Length:", len(body))
print("Connection: close")
print("Cache-Control: no-cache, no-store, must-revalidate")
print("Pragma: no-cache")
print("Expires: 0")
print("Set-Cookie: sessionid=abc123; HttpOnly; Path=/")
print("X-Powered-By: Python/3 CGI\n")

print("=== CGI chdir test ===")
cwd = os.getcwd()
script_path = os.path.abspath(__file__)
print(f"Current working directory: {cwd}")
print(f"Script absolute path: {script_path}\n")

print("Listing local files:\n")
files = os.listdir(".")
if files:
    for f in files:
        print(f"- {f}")
else:
    print("No files found.")

# Optional: try reading relative_test.txt
filename = "relative_test.txt"
if os.path.exists(filename):
    print(f"\nContents of {filename}:")
    with open(filename) as f:
        print(f.read())
else:
    print(f"\n{filename} does not exist.")
