
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

class HttpRequestTestGET : public ::testing::Test {
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
						"\r\n";
		}

		void TearDown() override {
			// Runs after EACH test
			delete request;
			delete client;
		}

		HttpRequest* request;
		std::string requestData;
		ClientInfo* client;
};

class HttpRequestTestPOST : public ::testing::Test {
	protected:
		void SetUp() override {
			// Runs before EACH test
			client = new ClientInfo();
			request = new HttpRequest();

			requestData = "POST /api/users HTTP/1.1\r\n"
						"Host: localhost:8080\r\n"
						"Content-Type: application/json\r\n"
						"Content-Length: 24\r\n"
						"\r\n"
						"{\"name\":\"John\",\"age\":30}";
		}

		void TearDown() override {
			// Runs after EACH test
			delete request;
			delete client;
		}

		HttpRequest* request;
		std::string requestData;
		ClientInfo* client;
};

#endif
