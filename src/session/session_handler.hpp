/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   session_handler.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: WebServ Team                                                         */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 00:00:00 by webserv          #+#    #+#             */
/*   Updated: 2025/12/02 00:00:00 by webserv         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SESSION_HANDLER_HPP
#define SESSION_HANDLER_HPP

#include "connection.hpp"
#include "session.hpp"
#include <string>

/*
 * SessionHandler - Handles login/logout requests
 * 
 * Static helper class for session-related endpoints:
 * - POST /api/login  : Authenticate and create session
 * - POST /api/logout : Destroy session and clear cookie
 */
class SessionHandler {
public:
	static bool handleLogin(Connection& conn);
	static bool handleLogout(Connection& conn);

private:
	SessionHandler();  // Static class, no instances
	
	static bool parseLoginForm(const std::string& body, std::string& username, std::string& password);
	static bool validateCredentials(const std::string& username, const std::string& password);
};

#endif
