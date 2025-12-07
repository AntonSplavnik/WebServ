#include "parser.hpp"
#include "../exceptions/config_exceptions.hpp"

Parser::Parser(const std::vector<Token>& tokens, const std::string& filename)
	: _tokens(tokens), _filename(filename), _pos(0) {}

Parser::~Parser() {}

const Token& Parser::peek() const {
	if (_pos >= _tokens.size()) {
		return _tokens.back(); // Return EOF token
	}
	return _tokens[_pos];
}

const Token& Parser::peekNext() const {
	if (_pos + 1 >= _tokens.size()) {
		return _tokens.back(); // Return EOF token
	}
	return _tokens[_pos + 1];
}

const Token& Parser::advance() {
	if (_pos < _tokens.size()) {
		return _tokens[_pos++];
	}
	return _tokens.back(); // Return EOF token
}

bool Parser::isAtEnd() const {
	return peek().type == Token::EOF_TOKEN;
}

bool Parser::match(Token::Type type) {
	if (check(type)) {
		advance();
		return true;
	}
	return false;
}

bool Parser::check(Token::Type type) const {
	if (isAtEnd()) return false;
	return peek().type == type;
}

void Parser::error(const std::string& message) const {
	const Token& token = peek();
	throw ParserException(message, token.line, token.column, _filename);
}

void Parser::error(const std::string& message, const Token& token) const {
	throw ParserException(message, token.line, token.column, _filename);
}

void Parser::cleanup(std::vector<Directive*>& directives) {
	for (size_t i = 0; i < directives.size(); ++i) {
		delete directives[i];
	}
	directives.clear();
}

std::vector<std::string> Parser::parseParameters() {
	std::vector<std::string> params;

	// Read parameters until we hit a block start, semicolon, or EOF
	while (!isAtEnd() &&
	       peek().type != Token::LBRACE &&
	       peek().type != Token::SEMICOLON) {

		const Token& token = peek();

		if (token.type == Token::WORD || token.type == Token::STRING) {
			params.push_back(advance().value);
		} else {
			error("Unexpected token in directive parameters");
		}
	}

	return params;
}

Directive* Parser::parseSimpleDirective() {
	const Token& nameToken = advance(); // Consume directive name
	Directive* directive = new Directive(nameToken.value, nameToken.line, nameToken.column);

	// Parse parameters
	directive->parameters = parseParameters();

	// Expect semicolon
	if (!match(Token::SEMICOLON)) {
		delete directive;
		error("Expected ';' after directive");
	}

	return directive;
}

Directive* Parser::parseBlockDirective() {
	const Token& nameToken = advance(); // Consume directive name
	Directive* directive = new Directive(nameToken.value, nameToken.line, nameToken.column);

	try {
		// Parse parameters (if any)
		directive->parameters = parseParameters();

		// Expect opening brace
		if (!match(Token::LBRACE)) {
			delete directive;
			error("Expected '{' to start block");
		}

		// Parse child directives until closing brace
		while (!isAtEnd() && !check(Token::RBRACE)) {
			Directive* child = parseDirective();
			if (child) {
				directive->children.push_back(child);
			}
		}

		// Expect closing brace
		if (!match(Token::RBRACE)) {
			delete directive;
			error("Expected '}' to close block");
		}

		return directive;

	} catch (...) {
		delete directive;
		throw;
	}
}

Directive* Parser::parseDirective() {
	if (isAtEnd()) {
		return NULL;
	}

	// Directive must start with a WORD token
	if (!check(Token::WORD)) {
		error("Expected directive name");
	}

	// Look ahead to determine if this is a block or simple directive
	// Block directive: WORD params* LBRACE
	// Simple directive: WORD params* SEMICOLON

	// We need to peek ahead to see if there's a LBRACE
	size_t savedPos = _pos;
	advance(); // Skip directive name

	// Skip parameters
	while (!isAtEnd() &&
	       peek().type != Token::LBRACE &&
	       peek().type != Token::SEMICOLON &&
	       peek().type != Token::RBRACE) {
		advance();
	}

	bool isBlock = check(Token::LBRACE);

	// Restore position
	_pos = savedPos;

	// Parse based on type
	if (isBlock) {
		return parseBlockDirective();
	} else {
		return parseSimpleDirective();
	}
}

std::vector<Directive*> Parser::parse() {
	std::vector<Directive*> directives;

	try {
		while (!isAtEnd()) {
			Directive* directive = parseDirective();
			if (directive) {
				directives.push_back(directive);
			}
		}
		return directives;

	} catch (...) {
		cleanup(directives);
		throw;
	}
}
