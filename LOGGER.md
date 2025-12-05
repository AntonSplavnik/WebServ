How to Use

  Default (INFO level):
  ./webserv working.conf
  Shows: INFO, WARNING, ERROR messages

  Debug mode (see everything):
  ./webserv working.conf --log-level debug
  Shows: DEBUG, INFO, WARNING, ERROR messages

  Production mode (only warnings/errors):
  ./webserv working.conf --log-level warning
  Shows: WARNING, ERROR messages only

  Silent mode (errors only):
  ./webserv working.conf --log-level error
  Shows: ERROR messages only

  Output Format

  - [ERROR] → stderr
  - [WARNING] → stderr
  - [INFO] → stdout
  - [DEBUG] → stdout

  # Production (only errors)
  ./webserv working.conf --log-level error

  # Normal operation (info + warnings + errors)
  ./webserv working.conf --log-level info

  # Development/debugging (everything)
  ./webserv working.conf --log-level debug
