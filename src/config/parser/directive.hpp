#ifndef DIRECTIVE_HPP
#define DIRECTIVE_HPP

#include <string>
#include <vector>

// Represents a single directive in the config tree
// Can be simple (e.g., "listen 8080;") or block (e.g., "server { ... }")
struct Directive {
	std::string name;                      // Directive name (e.g., "server", "listen", "location")
	std::vector<std::string> parameters;   // Parameters (e.g., ["8080"], ["/api"])
	std::vector<Directive*> children;      // Child directives (for block directives)
	size_t line;                           // Line number for error reporting
	size_t column;                         // Column number for error reporting

	Directive();
	Directive(const std::string& n, size_t ln, size_t col);
	~Directive();

	// Check if this is a block directive (has children)
	bool isBlock() const;

	// Find first child directive by name
	Directive* findChild(const std::string& name) const;

	// Find all child directives by name
	std::vector<Directive*> findChildren(const std::string& name) const;

	// Get parameter value at index (with bounds checking)
	std::string getParam(size_t index) const;

	// Check if has parameter at index
	bool hasParam(size_t index) const;

private:
	// Disable copy
	Directive(const Directive&);
	Directive& operator=(const Directive&);
};

#endif // DIRECTIVE_HPP
