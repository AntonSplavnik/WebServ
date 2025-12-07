#include "lexer.hpp"
#include "../exceptions/config_exceptions.hpp"
#include <cctype>

Lexer::Lexer(const std::string& input, const std::string& filename)
	: _input(input), _filename(filename), _pos(0), _line(1), _column(1) {}

char Lexer::peek() const {
	if (isAtEnd()) return '\0';
	return _input[_pos];
}

char Lexer::peekNext() const {
	if (_pos + 1 >= _input.length()) return '\0';
	return _input[_pos + 1];
}

char Lexer::advance() {
	if (isAtEnd()) return '\0';
	char c = _input[_pos++];
	if (c == '\n') {
		_line++;
		_column = 1;
	} else {
		_column++;
	}
	return c;
}

bool Lexer::isAtEnd() const {
	return _pos >= _input.length();
}

void Lexer::skipWhitespace() {
	while (!isAtEnd()) {
		char c = peek();
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
			advance();
		} else {
			break;
		}
	}
}

void Lexer::skipComment() {
	// Skip until end of line
	while (!isAtEnd() && peek() != '\n') {
		advance();
	}
}

Token Lexer::makeToken(Token::Type type, const std::string& value) {
	return Token(type, value, _line, _column - value.length());
}

Token Lexer::readWord() {
	size_t startLine = _line;
	size_t startColumn = _column;
	std::string word;

	// Word characters: alphanumeric, underscore, dash, dot, slash, colon
	// This allows: server_name, client-body-size, /path/to/file, 127.0.0.1:8080
	while (!isAtEnd()) {
		char c = peek();
		if (std::isalnum(c) || c == '_' || c == '-' || c == '.' ||
		    c == '/' || c == ':' || c == '*') {
			word += advance();
		} else {
			break;
		}
	}

	return Token(Token::WORD, word, startLine, startColumn);
}

Token Lexer::readString(char quote) {
	size_t startLine = _line;
	size_t startColumn = _column;
	std::string str;

	// Skip opening quote
	advance();

	while (!isAtEnd()) {
		char c = peek();

		if (c == quote) {
			advance(); // Skip closing quote
			return Token(Token::STRING, str, startLine, startColumn);
		}

		if (c == '\\') {
			advance();
			if (isAtEnd()) {
				error("Unexpected end of file in string");
			}
			char escaped = advance();
			// Handle escape sequences
			switch (escaped) {
				case 'n':  str += '\n'; break;
				case 't':  str += '\t'; break;
				case 'r':  str += '\r'; break;
				case '\\': str += '\\'; break;
				case '"':  str += '"'; break;
				case '\'': str += '\''; break;
				default:   str += escaped; break; // Pass through unknown escapes
			}
		} else {
			str += advance();
		}
	}

	error("Unterminated string");
	return Token(); // Never reached
}

void Lexer::error(const std::string& message) const {
	throw LexerException(message, _line, _column, _filename);
}

std::vector<Token> Lexer::tokenize() {
	std::vector<Token> tokens;

	while (!isAtEnd()) {
		skipWhitespace();
		if (isAtEnd()) break;

		char c = peek();

		// Comments
		if (c == '#') {
			skipComment();
			continue;
		}

		// Single-character tokens
		if (c == '{') {
			advance();
			tokens.push_back(makeToken(Token::LBRACE, "{"));
			continue;
		}

		if (c == '}') {
			advance();
			tokens.push_back(makeToken(Token::RBRACE, "}"));
			continue;
		}

		if (c == ';') {
			advance();
			tokens.push_back(makeToken(Token::SEMICOLON, ";"));
			continue;
		}

		// Quoted strings
		if (c == '"' || c == '\'') {
			tokens.push_back(readString(c));
			continue;
		}

		// Words (directive names, values)
		if (std::isalnum(c) || c == '_' || c == '-' || c == '.' ||
		    c == '/' || c == ':' || c == '*') {
			tokens.push_back(readWord());
			continue;
		}

		// Unknown character
		std::string msg = "Unexpected character: '";
		msg += c;
		msg += "'";
		error(msg);
	}

	// Add EOF token
	tokens.push_back(Token(Token::EOF_TOKEN, "", _line, _column));
	return tokens;
}
