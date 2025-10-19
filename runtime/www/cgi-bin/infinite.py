#!/usr/bin/env python3
import os
import sys
import time

print("Content-Type: text/plain\n")

print("Starting long-running CGI process...")
print("PID:", os.getpid())
print("Working directory:", os.getcwd())
print("Request method:", os.environ.get("REQUEST_METHOD", ""))
print("Script filename:", os.environ.get("SCRIPT_FILENAME", ""))

sys.stdout.flush()  # force output before sleeping

# Infinite loop — simulates a CGI that never finishes
i = 0
while True:
    i += 1
    print(f"Still running... iteration {i}")
    sys.stdout.flush()
    time.sleep(1)
