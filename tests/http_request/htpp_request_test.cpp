#include "http_request_test.hpp"


/*
  TEST(Example, Demo) {
      EXPECT_EQ(1, 2);  // Fails, prints error
      EXPECT_EQ(3, 3);  // Still runs! ✓

      ASSERT_EQ(4, 5);  // Fails, stops here
      EXPECT_EQ(6, 6);  // Never runs ✗
  }

  ---
  Common assertion macros:

  EXPECT_EQ(a, b)      // a == b
  EXPECT_NE(a, b)      // a != b
  EXPECT_LT(a, b)      // a < b
  EXPECT_LE(a, b)      // a <= b
  EXPECT_GT(a, b)      // a > b
  EXPECT_GE(a, b)      // a >= b

  EXPECT_TRUE(condition)
  EXPECT_FALSE(condition)

  EXPECT_STREQ(str1, str2)  // C-strings equal
  EXPECT_STRNE(str1, str2)  // C-strings not equal

  Rule of thumb: Use EXPECT_* for most checks, ASSERT_* when continuing the test makes no sense (e.g.,
  pointer is NULL).
*/

TEST_F(HttpRequestTestGET, extractLineHeaderBodyLen){

	request->extractLineHeaderBodyLen(requestData);

	EXPECT_STREQ("GET /index.html HTTP/1.1", request->getRequstLine().c_str());
	EXPECT_STREQ("Host: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\nAccept: text/html\r\nConnection: keep-alive",
				request->getRawHeaders().c_str());
}

TEST_F(HttpRequestTestGET, parseRequestLine){

	request->parseRequest(requestData);

	EXPECT_STREQ("GET", request->getMethod().c_str());
	EXPECT_STREQ("/index.html", request->getPath().c_str());
	EXPECT_STREQ("HTTP/1.1", request->getVersion().c_str());
}

TEST_F(HttpRequestTestGET, parseHeaders){

	request->parseRequest(requestData);

	const std::map<std::string, std::string>& headers = request->getHeaders();

	EXPECT_EQ(4u, headers.size());
	EXPECT_STREQ("localhost:8080", headers.at("host").c_str());
	EXPECT_STREQ("Mozilla/5.0", headers.at("user-agent").c_str());
	EXPECT_STREQ("text/html", headers.at("accept").c_str());
	EXPECT_STREQ("keep-alive", headers.at("connection").c_str());
}


TEST_F(HttpRequestTestPOST, parseBody){

	request->parseRequest(requestData);

	EXPECT_STREQ("{\"name\":\"John\",\"age\":30}", request->getBody().c_str());
}
