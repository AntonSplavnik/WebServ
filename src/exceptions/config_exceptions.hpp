#ifndef CONFIG_EXCEPTIONS_HPP
#define CONFIG_EXCEPTIONS_HPP

#include <iostream>

class ConfigParseException : public std::runtime_error
{
public:
	//The explicit keyword prevents the constructor from being used for implicit conversions or copy-initialization. Without explicit, code like ConfigParseException e = "error message"; would compile, converting a string to your exception. With explicit, only direct construction like ConfigParseException e("error message"); is allowed, making your code safer and less error-prone.
	explicit ConfigParseException(const std::string &msg)
		: std::runtime_error(msg)
	{}
};

#endif //CONFIG_EXCEPTIONS_HPP
