#!/usr/bin/env python3


import os
import sys
from urllib.parse import parse_qs

def parse_cookies(cookie_string):
    """Parse HTTP_COOKIE environment variable into dictionary"""
    cookies = {}
    if cookie_string:
        for item in cookie_string.split(';'):
            item = item.strip()
            if '=' in item:
                key, value = item.split('=', 1)
                cookies[key.strip()] = value.strip()
    return cookies

def main():
    # Read existing cookies
    cookie_string = os.environ.get('HTTP_COOKIE', '')
    cookies = parse_cookies(cookie_string)

    # Parse query string to see what action to take
    query_string = os.environ.get('QUERY_STRING', '')
    params = parse_qs(query_string)
    action = params.get('action', [''])[0]

    # Prepare response headers
    response_headers = []

    # Handle actions
    if action == 'set':
        response_headers.append('Set-Cookie: test_cookie=HelloWorld; Path=/')
        response_headers.append('Set-Cookie: session_id=12345; Path=/; HttpOnly')
        message = 'Cookies set! Refresh to see them.'
    elif action == 'update':
        response_headers.append('Set-Cookie: test_cookie=UpdatedValue; Path=/')
        message = 'Cookie updated! Refresh to see change.'
    elif action == 'delete':
        response_headers.append('Set-Cookie: test_cookie=; Path=/; Max-Age=0')
        response_headers.append('Set-Cookie: session_id=; Path=/; Max-Age=0')
        response_headers.append('Set-Cookie: cookie1=; Path=/; Max-Age=0')
        response_headers.append('Set-Cookie: cookie2=; Path=/; Max-Age=0')
        response_headers.append('Set-Cookie: cookie3=; Path=/; Max-Age=0')
        message = 'Cookies deleted! Refresh to confirm.'
    elif action == 'multiple':
        response_headers.append('Set-Cookie: cookie1=value1; Path=/')
        response_headers.append('Set-Cookie: cookie2=value2; Path=/')
        response_headers.append('Set-Cookie: cookie3=value3; Path=/')
        message = 'Multiple cookies set!'
    else:
        message = 'No action taken. Click buttons below to test.'

    # Build HTML response
    html = f"""<!DOCTYPE html>
<html>
<head>
    <title>Cookie Test - WebServ</title>
    <style>
        body {{
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
            background-color: #f5f5f5;
        }}
        .container {{
            background-color: white;
            padding: 30px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }}
        h1 {{
            color: #333;
            border-bottom: 2px solid #007bff;
            padding-bottom: 10px;
        }}
        .section {{
            margin: 20px 0;
            padding: 15px;
            background-color: #f9f9f9;
            border-left: 4px solid #007bff;
        }}
        .cookie-list {{
            background-color: #fff;
            padding: 15px;
            border: 1px solid #ddd;
            border-radius: 4px;
            margin: 10px 0;
        }}
        .cookie-item {{
            padding: 8px;
            margin: 5px 0;
            background-color: #e9ecef;
            border-radius: 3px;
        }}
        .buttons {{
            margin: 20px 0;
        }}
        button {{
            background-color: #007bff;
            color: white;
            border: none;
            padding: 10px 20px;
            margin: 5px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
        }}
        button:hover {{
            background-color: #0056b3;
        }}
        .delete-btn {{
            background-color: #dc3545;
        }}
        .delete-btn:hover {{
            background-color: #c82333;
        }}
        .message {{
            padding: 15px;
            margin: 10px 0;
            background-color: #d4edda;
            border: 1px solid #c3e6cb;
            border-radius: 4px;
            color: #155724;
        }}
        .env-info {{
            font-size: 12px;
            color: #666;
            font-family: monospace;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>🍪 Cookie Test - WebServ CGI</h1>

        <div class="message">{message}</div>

        <div class="section">
            <h2>Current Cookies</h2>
            <div class="cookie-list">
"""

    if cookies:
        for key, value in cookies.items():
            html += f'                <div class="cookie-item"><strong>{key}</strong> = {value}</div>\n'
    else:
        html += '                <div class="cookie-item"><em>No cookies set</em></div>\n'

    html += f"""            </div>
        </div>

        <div class="section">
            <h2>Test Actions</h2>
            <div class="buttons">
                <form method="GET" style="display: inline;">
                    <button type="submit" name="action" value="set">Set Test Cookies</button>
                </form>
                <form method="GET" style="display: inline;">
                    <button type="submit" name="action" value="update">Update Cookie</button>
                </form>
                <form method="GET" style="display: inline;">
                    <button type="submit" name="action" value="multiple">Set Multiple Cookies</button>
                </form>
                <form method="GET" style="display: inline;">
                    <button type="submit" name="action" value="delete" class="delete-btn">Delete All Cookies</button>
                </form>
            </div>
        </div>

        <div class="section">
            <h2>Environment Info</h2>
            <div class="env-info">
                <strong>HTTP_COOKIE:</strong> {cookie_string if cookie_string else '(empty)'}<br>
                <strong>QUERY_STRING:</strong> {query_string if query_string else '(empty)'}<br>
                <strong>REQUEST_METHOD:</strong> {os.environ.get('REQUEST_METHOD', 'N/A')}<br>
            </div>
        </div>

        <div class="section">
            <h2>Response Headers Sent</h2>
            <div class="env-info">
"""

    if response_headers:
        for header in response_headers:
            html += f'                {header}<br>\n'
    else:
        html += '                <em>No Set-Cookie headers sent this request</em><br>\n'

    html += """            </div>
        </div>
    </div>
</body>
</html>
"""

    # Output CGI response
    print("Content-Type: text/html")
    for header in response_headers:
        print(header)
    print()  # End of headers
    print(html)

if __name__ == '__main__':
    main()
