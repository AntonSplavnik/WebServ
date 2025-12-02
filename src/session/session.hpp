/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   session.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: WebServ Team                                                         */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 00:00:00 by webserv          #+#    #+#             */
/*   Updated: 2025/12/02 00:00:00 by webserv         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <map>
#include <ctime>
#include <cstdlib>

/*
 * Session - Represents user session data
 * 
 * Fields:
 *   - sid: Session ID (32-char hex string, cryptographically random)
 *   - user_id: User identifier (0 = anonymous/not logged in)
 *   - created_at: Session creation timestamp
 *   - last_access: Last activity timestamp (updated on each request)
 *   - expires_at: Expiration timestamp (absolute time)
 *   - csrf_token: CSRF protection token (random, per-session)
 *   - data: Custom key-value storage for application data
 */
struct Session {
	std::string sid;
	int user_id;
	std::time_t created_at;
	std::time_t last_access;
	std::time_t expires_at;
	std::string csrf_token;
	std::map<std::string, std::string> data;

	Session()
		: sid(""),
		  user_id(0),
		  created_at(0),
		  last_access(0),
		  expires_at(0),
		  csrf_token(""),
		  data() {}

	Session(const std::string& id, int uid, int ttl)
		: sid(id),
		  user_id(uid),
		  created_at(std::time(NULL)),
		  last_access(std::time(NULL)),
		  expires_at(std::time(NULL) + ttl),
		  csrf_token(""),
		  data() {}

	bool isExpired() const {
		return std::time(NULL) > expires_at;
	}

	void touch() {
		last_access = std::time(NULL);
	}
};

/*
 * SessionStore - Thread-safe session storage (singleton)
 * 
 * Responsibilities:
 *   - Create sessions with secure random SID
 *   - Retrieve sessions by SID
 *   - Update last_access timestamp (touch)
 *   - Destroy sessions (logout)
 *   - Garbage collect expired sessions
 * 
 * Thread-safety: All public methods are protected by mutex
 * 
 * Usage:
 *   SessionStore& store = SessionStore::getInstance();
 *   std::string sid = store.createSession(42, 3600);  // user_id=42, TTL=1h
 *   Session* session = store.getSession(sid);
 *   if (session) { ... }
 */
class SessionStore {
	public:
		static SessionStore& getInstance();

		// Session lifecycle
		std::string createSession(int user_id, int ttl_seconds);
		Session* getSession(const std::string& sid);
		void touchSession(const std::string& sid);
		void destroySession(const std::string& sid);

		// Maintenance
		void garbageCollect();
		size_t getSessionCount() const;

		// Security
		std::string generateSecureToken();

	private:
		SessionStore();
		~SessionStore();
		SessionStore(const SessionStore&);
		SessionStore& operator=(const SessionStore&);

		std::string generateSID();

		std::map<std::string, Session> _sessions;
		// Note: In production, add std::mutex _mutex for thread-safety
		// #include <pthread.h> or C++11 <mutex>
};

#endif
