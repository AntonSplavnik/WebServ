#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include <string>
#include <map>
#include <ctime>

struct SessionData {
	std::map<std::string, std::string> data;
	time_t lastAccess;

	SessionData() : lastAccess(time(NULL)) {}
};

class SessionManager {
private:
	std::map<std::string, SessionData> _sessions;
	int _sessionTimeout; // seconds
	time_t _lastCleanupTime;

	std::string generateSessionId();
	std::string generateRandomString(size_t length);

public:
	SessionManager();
	explicit SessionManager(int timeout);
	~SessionManager();

	// Session lifecycle
	std::string createSession();
	std::string getOrCreateSession(const std::string& sessionId);
	bool exists(const std::string& sessionId);
	void destroy(const std::string& sessionId);
	void cleanupExpired();

	// Data access
	void set(const std::string& sessionId, const std::string& key, const std::string& value);
	std::string get(const std::string& sessionId, const std::string& key);
	bool has(const std::string& sessionId, const std::string& key);
	void remove(const std::string& sessionId, const std::string& key);

	// Get all session data (for passing to CGI)
	std::map<std::string, std::string> getData(const std::string& sessionId);
	std::string getSessionCookieString(const std::string& sessionId) const;

	// Touch session (update last access time)
	void touch(const std::string& sessionId);
	void cleanupIfNeeded();
};

#endif
