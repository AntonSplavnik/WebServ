#ifndef LEXER_HPP
#define LEXER_HPP

#include "token.hpp"
#include <string>
#include <vector>

class Lexer {
public:
	explicit Lexer(const std::string& input, const std::string& filename = "");

	// Tokenize entire input
	std::vector<Token> tokenize();

private:
	const std::string& _input;
	const std::string _filename;
	size_t _pos;
	size_t _line;
	size_t _column;

	// Character access
	char peek() const;
	char peekNext() const;
	char advance();
	bool isAtEnd() const;

	// Skip helpers
	void skipWhitespace();
	void skipComment();

	// Token readers
	Token makeToken(Token::Type type, const std::string& value);
	Token readWord();
	Token readString(char quote);

	// Error handling
	void error(const std::string& message) const;
};

#endif // LEXER_HPP
