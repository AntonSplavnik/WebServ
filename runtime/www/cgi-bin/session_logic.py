#!/usr/bin/env python

import os
import cgi
import json

def get_all_session_data():
    """Gathers all SESSION_* environment variables into a dictionary."""
    session_data = {}
    for key, value in os.environ.items():
        if key.startswith('SESSION_'):
            # Clean up key for JSON output: remove prefix, lowercase, replace underscore
            clean_key = key[len('SESSION_'):].lower().replace('_', '')
            session_data[clean_key] = value
    return session_data

def set_session_var(key, value):
    """Prints the X-Session-Set header to request a server-side session update."""
    print(f"X-Session-Set: {key}={value}")

def main():
    """
    Acts as a simple JSON API for the frontend.
    1. Reads the 'action' from the query string.
    2. Performs session modifications by printing 'X-Session-Set' headers.
    3. Gathers all current session data from environment variables.
    4. Prints the session data back to the client as a JSON object.
    """
    form = cgi.FieldStorage()
    action = form.getvalue('action', 'getState') # Default action is to just get the state

    # --- Perform Actions & Update Session ---
    # Note: These 'set_session_var' calls only stage the changes.
    # The server applies them, and the *next* request will see the new values in os.environ.

    if action == 'login':
        username = form.getvalue('username', 'Anonymous')
        set_session_var('user_id', username)

    elif action == 'logout':
        set_session_var('user_id', 'Guest')

    elif action == 'setTheme':
        theme = form.getvalue('theme', 'light')
        set_session_var('theme', theme)

    elif action == 'setCustom':
        key = form.getvalue('key')
        value = form.getvalue('value')
        if key and value is not None:
            # A little sanitization for the key
            clean_key = ''.join(e for e in key if e.isalnum() or e == '_')
            set_session_var(clean_key, value)
    
    # --- Always Respond with Current State ---
    
    # This gathers the state as it was at the *beginning* of this request.
    current_session_state = get_all_session_data()

    # We can manually update the state for the *immediate* JSON response
    # to avoid the "one step behind" feeling on the frontend.
    if action == 'login':
        current_session_state['userid'] = form.getvalue('username', 'Anonymous')
    elif action == 'logout':
        current_session_state['userid'] = 'Guest'
    elif action == 'setTheme':
        current_session_state['theme'] = form.getvalue('theme', 'light')
    elif action == 'setCustom':
        key = form.getvalue('key')
        value = form.getvalue('value')
        if key and value is not None:
            clean_key = ''.join(e for e in key if e.isalnum() or e == '_').lower()
            current_session_state[clean_key] = value

    # Send the JSON response
    print("Content-Type: application/json")
    print() # End of headers
    print(json.dumps(current_session_state))

if __name__ == "__main__":
    main()

