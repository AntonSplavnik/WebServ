  Architecture

  Client Request → ConnectionPoolManager
      ↓
  CgiExecutor::setupSession()
      - Extract SESSID cookie
      - getOrCreateSession() → creates or validates
      - Store in connection._sessionId
      ↓
  Cgi::start()
      - Cache sessionManager.getData(sessionId) BEFORE fork
      ↓
  prepEnvVariables()
      - Build SESSION_key=value environment variables
      ↓
  CGI Script Execution
      - Reads os.environ['SESSION_*']
      - Outputs X-Session-Set: key=value headers
      ↓
  handleCGIread() → updateSessionFromCGI()
      - Parse X-Session-Set headers
      - sessionManager.set(sessionId, key, value)
      ↓
  prepareResponse(cgiOutput, sessionManager)
      - Add Set-Cookie: SESSID=...; HttpOnly; Max-Age=3600
      ↓
  Client receives response with updated cookie


  # Test in browser
  curl -c cookies.txt http://localhost:8080/cgi-bin/session_test.py
  curl -b cookies.txt http://localhost:8080/cgi-bin/session_test.py?action=login&username=Alice
  curl -b cookies.txt http://localhost:8080/cgi-bin/session_test.py
