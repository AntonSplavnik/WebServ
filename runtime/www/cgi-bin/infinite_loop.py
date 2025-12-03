#!/usr/bin/env python3
"""
infinite_loop.py
A minimal CGI script that sends an HTTP header then enters an infinite loop.
Use to test how the server handles a CGI that never finishes.
Make executable: chmod +x runtime/www/cgi-bin/infinite_loop.py
"""

import sys, time, signal

# graceful exit on SIGTERM
def _term(signum, frame):
    try:
        print("\n# received SIGTERM, exiting")
        sys.stdout.flush()
    finally:
        sys.exit(0)

signal.signal(signal.SIGTERM, _term)

# send HTTP header and flush so the server sees headers immediately
print("Content-Type: text/plain")
print()
sys.stdout.flush()

# simple infinite loop that periodically writes a dot and flushes
try:
    while True:
        print('.', end='', flush=True)
        time.sleep(1)
except KeyboardInterrupt:
    pass

