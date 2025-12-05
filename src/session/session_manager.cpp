#include "session_manager.hpp"
#include <cstdlib>
#include <sstream>
#include <iostream>

SessionManager::SessionManager() : _sessionTimeout(3600), _lastCleanupTime(time(NULL)) {} // Default 1 hour

SessionManager::SessionManager(int timeout) : _sessionTimeout(timeout), _lastCleanupTime(time(NULL)) {}

SessionManager::~SessionManager() {}

std::string SessionManager::generateRandomString(size_t length) {
	static const char chars[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	static const size_t charsLen = sizeof(chars) - 1;

	std::string result;
	result.reserve(length);

	for (size_t i = 0; i < length; ++i) {
		result += chars[rand() % charsLen];
	}

	return result;
}

std::string SessionManager::generateSessionId() {
	// Generate unique session ID
	std::string sessionId;
	do {
		sessionId = generateRandomString(32);
	} while (exists(sessionId));

	return sessionId;
}

std::string SessionManager::createSession() {
	std::string sessionId = generateSessionId();

	SessionData newSession;
	newSession.lastAccess = time(NULL);

	_sessions[sessionId] = newSession;

	std::cout << "[SESSION] Created session: " << sessionId << std::endl;
	return sessionId;
}

std::string SessionManager::getOrCreateSession(const std::string& sessionId) {
	if (sessionId.empty() || !exists(sessionId)) {
		// No session or invalid session - create new one
		std::string newSessionId = createSession();
		std::cout << "[SESSION] No valid session found, created: " << newSessionId << std::endl;
		return newSessionId;
	}

	// Valid session - touch it (sliding window timeout)
	touch(sessionId);
	std::cout << "[SESSION] Valid session touched: " << sessionId << std::endl;
	return sessionId;
}

bool SessionManager::exists(const std::string& sessionId) {
	return _sessions.find(sessionId) != _sessions.end();
}

void SessionManager::destroy(const std::string& sessionId) {
	std::map<std::string, SessionData>::iterator it = _sessions.find(sessionId);
	if (it != _sessions.end()) {
		_sessions.erase(it);
		std::cout << "[SESSION] Destroyed session: " << sessionId << std::endl;
	}
}

void SessionManager::cleanupExpired() {
	time_t now = time(NULL);
	std::map<std::string, SessionData>::iterator it = _sessions.begin();

	while (it != _sessions.end()) {
		if (now - it->second.lastAccess > _sessionTimeout) {
			std::cout << "[SESSION] Expired session: " << it->first << std::endl;
			_sessions.erase(it++);
		} else {
			++it;
		}
	}
}

void SessionManager::set(const std::string& sessionId, const std::string& key, const std::string& value) {
	if (!exists(sessionId)) {
		std::cerr << "[SESSION] Warning: Setting data on non-existent session: " << sessionId << std::endl;
		return;
	}

	_sessions[sessionId].data[key] = value;
	_sessions[sessionId].lastAccess = time(NULL);

	std::cout << "[SESSION] Set " << sessionId << "[" << key << "] = " << value << std::endl;
}

std::string SessionManager::get(const std::string& sessionId, const std::string& key) {
	if (!exists(sessionId)) {
		return "";
	}

	std::map<std::string, std::string>::iterator it = _sessions[sessionId].data.find(key);
	if (it != _sessions[sessionId].data.end()) {
		return it->second;
	}

	return "";
}

bool SessionManager::has(const std::string& sessionId, const std::string& key) {
	if (!exists(sessionId)) {
		return false;
	}

	return _sessions[sessionId].data.find(key) != _sessions[sessionId].data.end();
}

void SessionManager::remove(const std::string& sessionId, const std::string& key) {
	if (!exists(sessionId)) {
		return;
	}

	std::map<std::string, std::string>::iterator it = _sessions[sessionId].data.find(key);
	if (it != _sessions[sessionId].data.end()) {
		_sessions[sessionId].data.erase(it);
		std::cout << "[SESSION] Removed " << sessionId << "[" << key << "]" << std::endl;
	}
}

std::map<std::string, std::string> SessionManager::getData(const std::string& sessionId) {
	if (!exists(sessionId)) {
		return std::map<std::string, std::string>();
	}

	touch(sessionId);
	return _sessions[sessionId].data;
}

void SessionManager::touch(const std::string& sessionId) {
	if (exists(sessionId)) {
		_sessions[sessionId].lastAccess = time(NULL);
	}
}

std::string SessionManager::getSessionCookieString(const std::string& sessionId) const {
    if (sessionId.empty()) {
        return "";
    }

    std::stringstream ss;
    ss << "SESSID=" << sessionId;
    ss << "; HttpOnly; Path=/; Max-Age=" << _sessionTimeout;

    return ss.str();
}

void SessionManager::cleanupIfNeeded() {
    const int SESSION_CLEANUP_INTERVAL_SECONDS = 60; // Define cleanup interval
    time_t now = time(NULL);

    if (now - _lastCleanupTime >= SESSION_CLEANUP_INTERVAL_SECONDS) {
        cleanupExpired();
        _lastCleanupTime = now;
    }
}
