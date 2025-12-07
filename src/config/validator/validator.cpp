#include "validator.hpp"
#include "../exceptions/config_exceptions.hpp"
#include <map>
#include <set>
#include <sstream>
#include <climits>

// Helper to convert size_t to string (C++98 compatible)
static std::string toString(size_t value) {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

Validator::Validator(const std::string& filename)
	: _filename(filename), _rules(DirectiveRules::instance()) {}

void Validator::error(const std::string& message, const Directive* dir) const {
	throw ValidatorException(message, dir->line, dir->column, _filename);
}

void Validator::validateArguments(const Directive* dir, const DirectiveRule* rule) {
	size_t argCount = dir->parameters.size();

	if (argCount < rule->minArgs) {
		error("Directive '" + dir->name + "' requires at least " +
		     toString(rule->minArgs) + " argument(s), got " + toString(argCount), dir);
	}

	if (rule->maxArgs != SIZE_MAX && argCount > rule->maxArgs) {
		error("Directive '" + dir->name + "' accepts at most " +
		     toString(rule->maxArgs) + " argument(s), got " + toString(argCount), dir);
	}

	// Check block presence
	if (dir->isBlock() && !rule->allowBlock) {
		error("Directive '" + dir->name + "' cannot have a block", dir);
	}

	if (!dir->isBlock() && rule->allowBlock && rule->minArgs == 0) {
		// Some directives like "server" require a block
		if (dir->name == "server" || dir->name == "location") {
			error("Directive '" + dir->name + "' requires a block", dir);
		}
	}
}

void Validator::validateContext(const Directive* dir, DirectiveContext ctx) {
	const DirectiveRule* rule = _rules.getRule(dir->name);

	if (!rule) {
		error("Unknown directive: " + dir->name, dir);
	}

	if (!rule->isAllowedIn(ctx)) {
		std::string contextName;
		switch (ctx) {
			case CTX_MAIN:     contextName = "main"; break;
			case CTX_SERVER:   contextName = "server"; break;
			case CTX_LOCATION: contextName = "location"; break;
		}
		error("Directive '" + dir->name + "' is not allowed in " + contextName + " context", dir);
	}
}

void Validator::validateDirectiveValue(const Directive* dir) {
	// This is where we'd add specific value validation
	// For now, we'll just do basic checks

	// Example: validate autoindex values
	if (dir->name == "autoindex") {
		std::string value = dir->getParam(0);
		if (value != "on" && value != "off" && value != "true" &&
		    value != "false" && value != "1" && value != "0") {
			error("Invalid autoindex value '" + value + "' (expected: on/off/true/false/1/0)", dir);
		}
	}

	// Example: validate upload_enabled values
	if (dir->name == "upload_enabled") {
		std::string value = dir->getParam(0);
		if (value != "on" && value != "off" && value != "true" &&
		    value != "false" && value != "1" && value != "0") {
			error("Invalid upload_enabled value '" + value + "' (expected: on/off/true/false/1/0)", dir);
		}
	}

	// More specific validations can be added here
	// (port ranges, file paths, HTTP methods, etc.)
}

void Validator::checkDuplicates(const Directive* block) {
	std::map<std::string, int> counts;

	for (size_t i = 0; i < block->children.size(); ++i) {
		const Directive* child = block->children[i];
		const DirectiveRule* rule = _rules.getRule(child->name);

		if (rule && !rule->allowMultiple) {
			counts[child->name]++;
			if (counts[child->name] > 1) {
				error("Duplicate directive: " + child->name, child);
			}
		}
	}
}

void Validator::checkRequiredDirectives(const Directive* block, DirectiveContext ctx) {
	// Get all registered rules for this context
	std::set<std::string> required;
	std::set<std::string> found;

	// Collect required directives for this context
	// This is simplified - in real implementation, we'd iterate through all rules
	if (ctx == CTX_SERVER) {
		required.insert("listen");
		required.insert("root");
		required.insert("backlog");
		required.insert("client_max_body_size");
	}

	// Collect found directives
	for (size_t i = 0; i < block->children.size(); ++i) {
		found.insert(block->children[i]->name);
	}

	// Check for missing required directives
	for (std::set<std::string>::const_iterator it = required.begin();
	     it != required.end(); ++it) {
		if (found.find(*it) == found.end()) {
			error("Missing required directive: " + *it, block);
		}
	}
}

void Validator::validateLocationBlock(const Directive* location) {
	// Validate location path parameter
	if (location->parameters.empty()) {
		error("Location directive requires a path parameter", location);
	}

	std::string path = location->getParam(0);
	if (path.empty() || path[0] != '/') {
		error("Location path must start with '/': " + path, location);
	}

	// Check duplicates
	checkDuplicates(location);

	// Validate each child directive
	for (size_t i = 0; i < location->children.size(); ++i) {
		validateDirective(location->children[i], CTX_LOCATION);
	}
}

void Validator::validateServerBlock(const Directive* server) {
	// Check required directives
	checkRequiredDirectives(server, CTX_SERVER);

	// Check duplicates
	checkDuplicates(server);

	// Validate each child directive
	for (size_t i = 0; i < server->children.size(); ++i) {
		Directive* child = server->children[i];

		if (child->name == "location") {
			validateLocationBlock(child);
		} else {
			validateDirective(child, CTX_SERVER);
		}
	}
}

void Validator::validateDirective(const Directive* dir, DirectiveContext ctx) {
	// Check if directive is known
	const DirectiveRule* rule = _rules.getRule(dir->name);
	if (!rule) {
		error("Unknown directive: " + dir->name, dir);
	}

	// Validate context
	validateContext(dir, ctx);

	// Validate arguments
	validateArguments(dir, rule);

	// Validate specific values
	validateDirectiveValue(dir);
}

void Validator::validate(const std::vector<Directive*>& tree) {
	// Root level should only contain "server" directives
	for (size_t i = 0; i < tree.size(); ++i) {
		Directive* dir = tree[i];

		if (dir->name != "server") {
			error("Only 'server' blocks are allowed at root level, found: " + dir->name, dir);
		}

		validateDirective(dir, CTX_MAIN);
		validateServerBlock(dir);
	}

	// Check that we have at least one server block
	if (tree.empty()) {
		throw ValidatorException("Configuration file must contain at least one 'server' block", 0, 0, _filename);
	}
}
