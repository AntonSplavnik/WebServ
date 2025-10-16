#!/usr/bin/env python3
import os, sys

print("Content-Type: text/plain\n")

method = os.environ.get("REQUEST_METHOD", "")
query  = os.environ.get("QUERY_STRING", "")

if method == "GET":
    print("GET request with query:", query)

elif method == "POST":
    length = int(os.environ.get("CONTENT_LENGTH", 0))
    body = sys.stdin.read(length)
    print("POST body:", body)

elif method == "DELETE":
    print("DELETE called on:", query)
