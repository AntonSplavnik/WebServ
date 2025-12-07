#ifndef DIRECTIVE_RULES_HPP
#define DIRECTIVE_RULES_HPP

#include <string>
#include <map>

// Context in which a directive can appear
enum DirectiveContext {
	CTX_MAIN,      // Top-level (outside any block)
	CTX_SERVER,    // Inside server block
	CTX_LOCATION   // Inside location block
};

// Rule definition for a directive
struct DirectiveRule {
	std::string name;
	std::vector<DirectiveContext> contexts;  // Where this directive can appear
	size_t minArgs;                          // Minimum number of arguments
	size_t maxArgs;                          // Maximum number of arguments (SIZE_MAX = unlimited)
	bool allowBlock;                         // Can this directive have a block?
	bool required;                           // Is this directive required in its context?
	bool allowMultiple;                      // Can appear multiple times?

	DirectiveRule();
	DirectiveRule(const std::string& n,
	             DirectiveContext ctx,
	             size_t minA,
	             size_t maxA,
	             bool block,
	             bool req,
	             bool multi);

	// Check if directive is allowed in given context
	bool isAllowedIn(DirectiveContext ctx) const;

	// Add a context to allowed contexts
	void addContext(DirectiveContext ctx);
};

// Registry of all directive rules
class DirectiveRules {
public:
	static DirectiveRules& instance();

	// Get rule for a directive (returns NULL if not found)
	const DirectiveRule* getRule(const std::string& name) const;

	// Check if directive exists
	bool hasRule(const std::string& name) const;

	// Check if directive is allowed in context
	bool isAllowedInContext(const std::string& name, DirectiveContext ctx) const;

private:
	DirectiveRules();
	std::map<std::string, DirectiveRule> _rules;

	void registerRule(const DirectiveRule& rule);
	void initializeRules();
};

#endif // DIRECTIVE_RULES_HPP
