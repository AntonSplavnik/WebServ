#!/usr/bin/env python3
import os
import sys

# Read query parameter to decide test type
# Example: /test_memory.py?mode=normal
# Example: /test_memory.py?mode=large_output
# Example: /test_memory.py?mode=memory_bomb

query_string = os.environ.get('QUERY_STRING', '')

print("Content-Type: text/html\r")
print("\r")

if 'mode=memory_bomb' in query_string:
    # Try to allocate huge memory (should be killed by setrlimit)
    print("<h1>Memory Bomb Test</h1>")
    print("<p>Attempting to allocate 200MB...</p>")
    sys.stdout.flush()

    try:
        # This should fail with setrlimit
        huge_list = []
        for i in range(1000000):
            huge_list.append('x' * 10000)  # 10KB per iteration = ~10GB total
        print("<p>ERROR: Should have been killed!</p>")
    except MemoryError:
        print("<p>MemoryError caught (setrlimit working!)</p>")

elif 'mode=large_output' in query_string:
    # Generate output exceeding MAX_CGI_OUTPUT
    print("<h1>Large Output Test</h1>")
    print("<p>Generating 20MB of output...</p>")
    sys.stdout.flush()

    # Generate 20MB of data
    chunk = "X" * 1024  # 1KB
    for i in range(20 * 1024):  # 20MB
        print(chunk)

elif 'mode=slow' in query_string:
    # Slow script to test timeout
    import time
    print("<h1>Slow Script Test</h1>")
    print("<p>Sleeping for 25 seconds (should timeout)...</p>")
    sys.stdout.flush()
    time.sleep(25)
    print("<p>Finished sleeping</p>")

else:
    # Normal operation
    print("<html>")
    print("<head><title>CGI Test - Python</title></head>")
    print("<body>")
    print("<h1>CGI Script Working!</h1>")
    print("<h2>Environment Variables:</h2>")
    print("<ul>")
    print(f"<li>REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'N/A')}</li>")
    print(f"<li>QUERY_STRING: {os.environ.get('QUERY_STRING', 'N/A')}</li>")
    print(f"<li>CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH', 'N/A')}</li>")
    print(f"<li>SCRIPT_FILENAME: {os.environ.get('SCRIPT_FILENAME', 'N/A')}</li>")
    print("</ul>")

    # For POST requests, read body
    content_length = os.environ.get('CONTENT_LENGTH')
    if content_length and content_length != '0':
        print("<h2>POST Body:</h2>")
        print("<pre>")
        body = sys.stdin.read(int(content_length))
        print(body)
        print("</pre>")

    print("<h2>Test Links:</h2>")
    print("<ul>")
    print(f"<li><a href='?mode=normal'>Normal Output</a></li>")
    print(f"<li><a href='?mode=memory_bomb'>Memory Bomb (test setrlimit)</a></li>")
    print(f"<li><a href='?mode=large_output'>Large Output (test MAX_CGI_OUTPUT)</a></li>")
    print(f"<li><a href='?mode=slow'>Slow Script (test timeout)</a></li>")
    print("</ul>")
    print("</body>")
    print("</html>")
