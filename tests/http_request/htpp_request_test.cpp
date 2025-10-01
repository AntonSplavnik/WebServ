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

TEST_F(HttpRequestTest, getMethod){

	std::string method = request->getMethod();
	EXPECT_STREQ("GET", method.c_str());
}

TEST_F(HttpRequestTest ,extractLineHeaderBodyLen){

	request->extractLineHeaderBodyLen(requestData);
	EXPECT_STREQ("GET /index.html HTTP/1.1\r\n", request->getRequestLine());
	EXPECT_STREQ("Host: localhost:8080\r\n"
				 "User-Agent: Mozilla/5.0\r\n"
				 "Accept: text/html\r\n"
				 "Connection: keep-alive\r\n"
				"\r\n",
				request->getRawHeaders());
}

/*
void HttpRequest::extractLineHeaderBodyLen(ClientInfo& requestDate){

	(void)requestDate;
	extraction logic
	_line =
	_rawHeders =
	_body =
	_contentLength =
}
*/
