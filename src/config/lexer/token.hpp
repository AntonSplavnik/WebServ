#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

struct Token {
	enum Type {
		WORD,         // Unquoted word (directive names, values)
		STRING,       // Quoted string
		LBRACE,       // {
		RBRACE,       // }
		SEMICOLON,    // ;
		EOF_TOKEN     // End of file
	};

	Type type;
	std::string value;
	size_t line;
	size_t column;

	Token();
	Token(Type t, const std::string& val, size_t ln, size_t col);

	// Human-readable type name for debugging
	std::string typeName() const;
};

#endif // TOKEN_HPP
