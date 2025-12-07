#ifndef VALIDATOR_HPP
#define VALIDATOR_HPP

#include "../parser/directive.hpp"
#include "directive_rules.hpp"
#include <vector>
#include <string>

class Validator {
public:
	Validator(const std::string& filename = "");

	// Validate directive tree
	// Throws ValidatorException on error
	void validate(const std::vector<Directive*>& tree);

private:
	const std::string _filename;
	DirectiveRules& _rules;

	// Validate a single directive in context
	void validateDirective(const Directive* dir, DirectiveContext ctx);

	// Validate server block
	void validateServerBlock(const Directive* server);

	// Validate location block
	void validateLocationBlock(const Directive* location);

	// Check required directives are present
	void checkRequiredDirectives(const Directive* block, DirectiveContext ctx);

	// Check for duplicate directives (where not allowed)
	void checkDuplicates(const Directive* block);

	// Validate directive argument count
	void validateArguments(const Directive* dir, const DirectiveRule* rule);

	// Validate directive context
	void validateContext(const Directive* dir, DirectiveContext ctx);

	// Validate specific directive values
	void validateDirectiveValue(const Directive* dir);

	// Error reporting
	void error(const std::string& message, const Directive* dir) const;
};

#endif // VALIDATOR_HPP
