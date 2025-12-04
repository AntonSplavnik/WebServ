#!/usr/bin/env python3
import time
import sys

print("Content-Type: text/plain\r")
print("\r")
sys.stdout.flush()

time.sleep(50)  # Sleep longer than CGI_TIMEOUT (40s)

print("This should never be sent")
