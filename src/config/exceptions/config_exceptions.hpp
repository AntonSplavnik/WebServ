#ifndef CONFIG_EXCEPTIONS_HPP
#define CONFIG_EXCEPTIONS_HPP

#include <string>
#include <sstream>
#include <stdexcept>

class ConfigException : public std::runtime_error {
protected:
	size_t _line;
	size_t _column;
	std::string _file;
	std::string _fullMessage;

	void buildMessage(const std::string& msg) {
		std::ostringstream oss;
		if (!_file.empty()) {
			oss << _file << ":";
		}
		oss << _line << ":" << _column << ": " << msg;
		_fullMessage = oss.str();
	}

public:
	ConfigException(const std::string& msg, size_t line, size_t col, const std::string& file = "")
		: std::runtime_error(msg), _line(line), _column(col), _file(file) {
		buildMessage(msg);
	}

	virtual ~ConfigException() throw() {}

	virtual const char* what() const throw() {
		return _fullMessage.c_str();
	}

	size_t getLine() const { return _line; }
	size_t getColumn() const { return _column; }
	const std::string& getFile() const { return _file; }
};

class LexerException : public ConfigException {
public:
	LexerException(const std::string& msg, size_t line, size_t col, const std::string& file = "")
		: ConfigException(msg, line, col, file) {}

	virtual ~LexerException() throw() {}
};

class ParserException : public ConfigException {
public:
	ParserException(const std::string& msg, size_t line, size_t col, const std::string& file = "")
		: ConfigException(msg, line, col, file) {}

	virtual ~ParserException() throw() {}
};

class ValidatorException : public ConfigException {
public:
	ValidatorException(const std::string& msg, size_t line, size_t col, const std::string& file = "")
		: ConfigException(msg, line, col, file) {}

	virtual ~ValidatorException() throw() {}
};

#endif // CONFIG_EXCEPTIONS_HPP
