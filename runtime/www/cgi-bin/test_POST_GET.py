#!/usr/bin/env python3
import os

print("Content-Type: text/plain\n")

# Only the vars that your C++ server should set
vars_to_check = [
    "GATEWAY_INTERFACE",
    "SERVER_PROTOCOL",
    "SERVER_SOFTWARE",
    "SERVER_NAME",
    "SERVER_PORT",
    "REQUEST_METHOD",
    "SCRIPT_NAME",
    "SCRIPT_FILENAME",
    "QUERY_STRING",
    "CONTENT_TYPE",
    "CONTENT_LENGTH",
    "PATH_INFO",
    "PATH_TRANSLATED",
    "REMOTE_ADDR",
    "REMOTE_PORT",
]

print("=== Selected CGI Environment Variables ===\n")

for var in vars_to_check:
    val = os.environ.get(var)
    if val is not None:
        print(f"{var} = {val}")
    else:
        print(f"{var} = [NOT SET]")
