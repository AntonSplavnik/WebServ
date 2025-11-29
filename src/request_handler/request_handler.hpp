#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "post_handler.hpp"

class RequestHandler {

	public:
		RequestHandler(){}
		~RequestHandler(){}

		void handleGET(Connection& connection);
		void handlePOST(Connection& connection);
		void handleDELETE(Connection& connection);
		void handleRedirect(Connection& connection);
};

#endif
