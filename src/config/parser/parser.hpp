#ifndef PARSER_HPP
#define PARSER_HPP

#include "directive.hpp"
#include "../lexer/token.hpp"
#include <vector>

class Parser {
public:
	explicit Parser(const std::vector<Token>& tokens, const std::string& filename = "");
	~Parser();

	// Parse tokens into directive tree
	// Returns root-level directives (typically "server" blocks)
	std::vector<Directive*> parse();

private:
	const std::vector<Token>& _tokens;
	const std::string _filename;
	size_t _pos;

	// Token access
	const Token& peek() const;
	const Token& peekNext() const;
	const Token& advance();
	bool isAtEnd() const;
	bool match(Token::Type type);
	bool check(Token::Type type) const;

	// Parsing
	Directive* parseDirective();
	Directive* parseSimpleDirective();
	Directive* parseBlockDirective();
	std::vector<std::string> parseParameters();

	// Error handling
	void error(const std::string& message) const;
	void error(const std::string& message, const Token& token) const;

	// Cleanup
	void cleanup(std::vector<Directive*>& directives);
};

#endif // PARSER_HPP
