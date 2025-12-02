/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   session_handler.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: WebServ Team                                                         */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 00:00:00 by webserv          #+#    #+#             */
/*   Updated: 2025/12/02 00:00:00 by webserv         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "session_handler.hpp"
#include "response.hpp"
#include <sstream>
#include <iostream>

/*
 * Parse application/x-www-form-urlencoded body
 * 
 * Example: "username=admin&password=secret"
 */
bool SessionHandler::parseLoginForm(const std::string& body, std::string& username, std::string& password) {
	username.clear();
	password.clear();

	std::istringstream iss(body);
	std::string pair;

	while (std::getline(iss, pair, '&')) {
		size_t pos = pair.find('=');
		if (pos == std::string::npos) continue;

		std::string key = pair.substr(0, pos);
		std::string value = pair.substr(pos + 1);

		if (key == "username") {
			username = value;
		} else if (key == "password") {
			password = value;
		}
	}

	return !username.empty() && !password.empty();
}

/*
 * Validate user credentials
 * 
 * For demo: hardcoded admin/secret
 * In production: check against database with bcrypt
 */
bool SessionHandler::validateCredentials(const std::string& username, const std::string& password) {
	// Demo credentials
	return (username == "admin" && password == "secret");
}

/*
 * Handle POST /api/login
 * 
 * Flow:
 * 1. Parse username/password from POST body
 * 2. Validate credentials
 * 3. Create session with 1h TTL
 * 4. Set secure cookie
 * 5. Redirect to /admin.html
 */
bool SessionHandler::handleLogin(Connection& conn) {
	const HttpRequest& request = conn.getRequest();
	std::string body = request.getBody();

	std::cout << "[SESSION] Login attempt from " << conn.getIp() << std::endl;

	// Parse form data
	std::string username, password;
	if (!parseLoginForm(body, username, password)) {
		std::cout << "[SESSION] Invalid form data" << std::endl;
		conn.setStatusCode(400);
		conn.setBodyContent("<html><body><h1>400 Bad Request</h1><p>Invalid form data</p></body></html>");
		return conn.prepareResponse();
	}

	std::cout << "[SESSION] Username: " << username << std::endl;

	// Validate credentials
	if (!validateCredentials(username, password)) {
		std::cout << "[SESSION] Invalid credentials" << std::endl;
		conn.setStatusCode(401);
		conn.setBodyContent("<html><body><h1>401 Unauthorized</h1><p>Invalid username or password</p><a href='/login.html'>Try again</a></body></html>");
		return conn.prepareResponse();
	}

	// Create session (user_id=42 for demo, TTL=3600s = 1h)
	SessionStore& store = SessionStore::getInstance();
	std::string sid = store.createSession(42, 3600);

	std::cout << "[SESSION] Session created: " << sid << std::endl;

	// Attach session to connection
	Session* session = store.getSession(sid);
	conn.setSession(session);

	// Prepare response with cookie
	conn.setStatusCode(303);  // See Other (redirect after POST)
	conn.setRedirectUrl("/admin.html");
	conn.prepareResponse();

	// Add session cookie to response
	HttpResponse response;
	response.setCookie("webserv_sid", sid, 3600, true, false, "Lax");
	
	// Get the Set-Cookie header and add to response data
	std::string responseData = conn.getRequest().getProtocolVersion() + " 303 See Other\r\n";
	responseData += "Location: /admin.html\r\n";
	responseData += "Set-Cookie: webserv_sid=" + sid + "; Max-Age=3600; HttpOnly; SameSite=Lax; Path=/\r\n";
	responseData += "Content-Length: 0\r\n";
	responseData += "Connection: close\r\n";
	responseData += "\r\n";

	conn.setResponseData(responseData);

	std::cout << "[SESSION] Login successful, redirecting to /admin.html" << std::endl;
	return true;
}

/*
 * Handle POST /api/logout
 * 
 * Flow:
 * 1. Get session ID from cookie
 * 2. Destroy session in store
 * 3. Clear cookie (Max-Age=0)
 * 4. Redirect to /login.html
 */
bool SessionHandler::handleLogout(Connection& conn) {
	const HttpRequest& request = conn.getRequest();
	std::string sid = request.getCookie("webserv_sid");

	std::cout << "[SESSION] Logout from " << conn.getIp() << std::endl;

	if (!sid.empty()) {
		SessionStore& store = SessionStore::getInstance();
		store.destroySession(sid);
		std::cout << "[SESSION] Session destroyed: " << sid << std::endl;
	}

	// Clear session pointer
	conn.setSession(NULL);

	// Prepare redirect response
	std::string responseData = request.getProtocolVersion() + " 303 See Other\r\n";
	responseData += "Location: /login.html\r\n";
	responseData += "Set-Cookie: webserv_sid=; Max-Age=0; Path=/\r\n";
	responseData += "Content-Length: 0\r\n";
	responseData += "Connection: close\r\n";
	responseData += "\r\n";

	conn.setResponseData(responseData);

	std::cout << "[SESSION] Logout successful, redirecting to /login.html" << std::endl;
	return true;
}
