#!/usr/bin/env python

import os
import cgi
import datetime

# --- Configuration ---
SESSION_COOKIE_NAME = "SESSID"
SESSION_TIMEOUT_SECONDS = 3600 # Should match server config

def get_session_id_from_cookie():
    """Extracts session ID from HTTP_COOKIE environment variable."""
    http_cookie = os.environ.get('HTTP_COOKIE', '')
    cookies = {}
    for cookie_pair in http_cookie.split(';'):
        if '=' in cookie_pair:
            key, value = cookie_pair.strip().split('=', 1)
            cookies[key] = value
    return cookies.get(SESSION_COOKIE_NAME)

def get_session_var(key, default_value=None):
    """Gets a session variable from environment (simulating server injection)."""
    env_key = f"SESSION_{key.upper()}"
    return os.environ.get(env_key, default_value)

def set_session_var(key, value):
    """Sets a session variable by printing X-Session-Set header."""
    print(f"X-Session-Set: {key}={value}")

def main():
    form = cgi.FieldStorage()
    action = form.getvalue('action', 'view')

    session_id = get_session_id_from_cookie()
    
    # Get existing session data
    user_id = get_session_var('user_id', 'Guest')
    visit_count = int(get_session_var('visit_count', 0))
    last_visit = get_session_var('last_visit', 'Never')

    # Update session data
    visit_count += 1
    current_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    set_session_var('visit_count', str(visit_count))
    set_session_var('last_visit', current_time)

    if action == 'login':
        new_user = form.getvalue('username', 'Anonymous')
        set_session_var('user_id', new_user)
        user_id = new_user
    elif action == 'logout':
        set_session_var('user_id', 'Guest')
        user_id = 'Guest'
    elif action == 'set_custom':
        custom_key = form.getvalue('custom_key', '')
        custom_value = form.getvalue('custom_value', '')
        if custom_key:
            set_session_var(custom_key, custom_value)
            
    # --- Print HTTP Headers ---
    print("Content-Type: text/html")
    # The server will automatically add Set-Cookie: SESSID=... based on the Connection's sessionId
    print() # End of headers

    # --- Print HTML Body ---
    print("<!DOCTYPE html>")
    print("<html>")
    print("<head><title>Session Test CGI</title>")
    print("<style>")
    print("  body { font-family: sans-serif; margin: 2em; background-color: #f4f4f4; color: #333; }")
    print("  .container { background-color: #fff; padding: 2em; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); max-width: 600px; margin: 0 auto; }")
    print("  h1 { color: #0056b3; }")
    print("  pre { background-color: #e9e9e9; padding: 1em; border-radius: 4px; overflow-x: auto; }")
    print("  form { margin-top: 1.5em; padding-top: 1em; border-top: 1px solid #eee; }")
    print("  label { display: block; margin-bottom: 0.5em; font-weight: bold; }")
    print("  input[type='text'], input[type='submit'] { width: calc(100% - 22px); padding: 10px; margin-bottom: 1em; border: 1px solid #ddd; border-radius: 4px; }")
    print("  input[type='submit'] { background-color: #007bff; color: white; border: none; cursor: pointer; }")
    print("  input[type='submit']:hover { background-color: #0056b3; }")
    print("  .note { color: #555; font-size: 0.9em; margin-top: 1em; }")
    print("</style>")
    print("</head>")
    print("<body>")
    print("  <div class='container'>")
    print(f"    <h1>Session Test for {user_id}</h1>")
    print(f"    <p>Session ID: <strong>{session_id if session_id else 'NOT SET (or not yet sent by browser)'}</strong></p>")
    print("    <h2>Session Data:</h2>")
    print("    <pre>")
    print(f"User ID: {user_id}")
    print(f"Visit Count: {visit_count}")
    print(f"Last Visit: {last_visit}")
    
    # Display other session variables from env
    for key, value in os.environ.items():
        if key.startswith('SESSION_') and key not in ['SESSION_USER_ID', 'SESSION_VISIT_COUNT', 'SESSION_LAST_VISIT']:
            print(f"{key.replace('SESSION_', '').replace('_', ' ').title()}: {value}")
    print("    </pre>")

    print("    <form method='GET' action='session_test.py'>")
    print("      <input type='hidden' name='action' value='login'>")
    print("      <label for='username'>Login as:</label>")
    print("      <input type='text' id='username' name='username' value='Tester'>")
    print("      <input type='submit' value='Login / Change User'>")
    print("    </form>")

    print("    <form method='GET' action='session_test.py'>")
    print("      <input type='hidden' name='action' value='logout'>")
    print("      <input type='submit' value='Logout'>")
    print("    </form>")

    print("    <form method='GET' action='session_test.py'>")
    print("      <input type='hidden' name='action' value='set_custom'>")
    print("      <label for='custom_key'>Set Custom Session Var (Key):</label>")
    print("      <input type='text' id='custom_key' name='custom_key' value='my_key'>")
    print("      <label for='custom_value'>Set Custom Session Var (Value):</label>")
    print("      <input type='text' id='custom_value' name='custom_value' value='my_value'>")
    print("      <input type='submit' value='Set Custom Var'>")
    print("    </form>")

    print("    <div class='note'>")
    print("      Note: The server will automatically manage the SESSID cookie and its expiry. ")
    print("      Check your browser's developer tools (Network tab) to see the 'Set-Cookie' header.")
    print("      To test expiry, wait more than 60 seconds (or your configured timeout) after your last visit, then reload.")
    print("    </div>")
    print("  </div>")
    print("</body>")
    print("</html>")

if __name__ == "__main__":
    main()
