#include "token.hpp"

Token::Token() : type(EOF_TOKEN), value(""), line(0), column(0) {}

Token::Token(Type t, const std::string& val, size_t ln, size_t col)
	: type(t), value(val), line(ln), column(col) {}

std::string Token::typeName() const {
	switch (type) {
		case WORD:       return "WORD";
		case STRING:     return "STRING";
		case LBRACE:     return "LBRACE";
		case RBRACE:     return "RBRACE";
		case SEMICOLON:  return "SEMICOLON";
		case EOF_TOKEN:  return "EOF";
		default:         return "UNKNOWN";
	}
}
