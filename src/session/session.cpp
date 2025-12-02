/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   session.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: WebServ Team                                                         */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 00:00:00 by webserv          #+#    #+#             */
/*   Updated: 2025/12/02 00:00:00 by webserv         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "session.hpp"
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>

/*
 * Singleton instance
 */
SessionStore& SessionStore::getInstance() {
	static SessionStore instance;
	return instance;
}

SessionStore::SessionStore() : _sessions() {
	// Seed random generator (C++98 compatible)
	std::srand(std::time(NULL));
}

SessionStore::~SessionStore() {
	_sessions.clear();
}

/*
 * Generate cryptographically secure Session ID
 * 
 * Uses /dev/urandom for randomness (POSIX systems)
 * Falls back to rand() if /dev/urandom unavailable
 * 
 * Format: 32 hexadecimal characters (128 bits of entropy)
 */
std::string SessionStore::generateSID() {
	std::ostringstream oss;
	oss << std::hex << std::setfill('0');

	int fd = open("/dev/urandom", O_RDONLY);
	if (fd != -1) {
		// Use /dev/urandom (cryptographically secure)
		unsigned char buffer[16];
		if (read(fd, buffer, 16) == 16) {
			for (int i = 0; i < 16; i++) {
				oss << std::setw(2) << static_cast<int>(buffer[i]);
			}
			close(fd);
			return oss.str();
		}
		close(fd);
	}

	// Fallback to rand() (less secure, but C++98 compatible)
	for (int i = 0; i < 16; i++) {
		oss << std::setw(2) << (std::rand() % 256);
	}
	return oss.str();
}

/*
 * Generate secure random token (for CSRF)
 */
std::string SessionStore::generateSecureToken() {
	return generateSID();  // Reuse SID generation logic
}

/*
 * Create new session
 * 
 * @param user_id: User identifier (0 for anonymous)
 * @param ttl_seconds: Time-to-live in seconds
 * @return: Session ID (sid)
 */
std::string SessionStore::createSession(int user_id, int ttl_seconds) {
	std::string sid = generateSID();
	
	// Ensure uniqueness (extremely unlikely collision, but safe)
	while (_sessions.find(sid) != _sessions.end()) {
		sid = generateSID();
	}

	Session session(sid, user_id, ttl_seconds);
	session.csrf_token = generateSecureToken();
	
	_sessions[sid] = session;
	return sid;
}

/*
 * Retrieve session by SID
 * 
 * @param sid: Session ID
 * @return: Pointer to Session (NULL if not found or expired)
 */
Session* SessionStore::getSession(const std::string& sid) {
	std::map<std::string, Session>::iterator it = _sessions.find(sid);
	
	if (it == _sessions.end()) {
		return NULL;  // Session not found
	}

	if (it->second.isExpired()) {
		_sessions.erase(it);  // Auto-cleanup on access
		return NULL;
	}

	return &(it->second);
}

/*
 * Update last_access timestamp (called on each request)
 */
void SessionStore::touchSession(const std::string& sid) {
	Session* session = getSession(sid);
	if (session) {
		session->touch();
	}
}

/*
 * Destroy session (logout)
 */
void SessionStore::destroySession(const std::string& sid) {
	_sessions.erase(sid);
}

/*
 * Remove all expired sessions
 * 
 * Should be called periodically (e.g., every 5 minutes)
 * from event loop or background thread
 */
void SessionStore::garbageCollect() {
	std::time_t now = std::time(NULL);
	std::map<std::string, Session>::iterator it = _sessions.begin();
	
	while (it != _sessions.end()) {
		if (it->second.expires_at < now) {
			std::map<std::string, Session>::iterator toErase = it;
			++it;
			_sessions.erase(toErase);
		} else {
			++it;
		}
	}
}

/*
 * Get current session count (for monitoring)
 */
size_t SessionStore::getSessionCount() const {
	return _sessions.size();
}
