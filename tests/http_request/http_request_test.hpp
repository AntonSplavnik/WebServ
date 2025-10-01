
#ifndef HTTP_REQUEST_TEST_HPP
#define HTTP_REQUEST_TEST_HPP

#include <iostream>
#include <string>
#include <gtest/gtest.h>
#include "http_request.hpp"

// Minimal ClientInfo for testing - only the fields needed
struct ClientInfo {
	std::string requestData;
};

class HttpRequestTest : public ::testing::Test {
	protected:
		void SetUp() override {
			// Runs before EACH test
			client = new ClientInfo();

			request = new HttpRequest();
			requestData = "GET /index.html HTTP/1.1\r\n"
						"Host: localhost:8080\r\n"
						"User-Agent: Mozilla/5.0\r\n"
						"Accept: text/html\r\n"
						"Connection: keep-alive\r\n"
						"\r\n";;


			// client->requestData = requestData;
			// request->setRequstLine("GET /index.html HTTP/1.1\r\n");
			// request->setRawHeaders("Host: localhost:8080\r\n"
			// 					   "User-Agent: Mozilla/5.0\r\n"
			// 					   "Accept: text/html\r\n"
			// 					   "Connection: keep-alive\r\n"
			// 					   "\r\n");
		}

		void TearDown() override {
			// Runs after EACH test
			delete request;
		}

		HttpRequest* request;
		std::string requestData;
		ClientInfo* client;
};

#endif
