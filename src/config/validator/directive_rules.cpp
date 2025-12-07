#include "directive_rules.hpp"
#include <climits>

DirectiveRule::DirectiveRule()
	: name(""), contexts(), minArgs(0), maxArgs(0),
	  allowBlock(false), required(false), allowMultiple(false) {}

DirectiveRule::DirectiveRule(const std::string& n,
                            DirectiveContext ctx,
                            size_t minA,
                            size_t maxA,
                            bool block,
                            bool req,
                            bool multi)
	: name(n), contexts(), minArgs(minA), maxArgs(maxA),
	  allowBlock(block), required(req), allowMultiple(multi) {
	contexts.push_back(ctx);
}

bool DirectiveRule::isAllowedIn(DirectiveContext ctx) const {
	for (size_t i = 0; i < contexts.size(); ++i) {
		if (contexts[i] == ctx) {
			return true;
		}
	}
	return false;
}

void DirectiveRule::addContext(DirectiveContext ctx) {
	// Check if not already present
	for (size_t i = 0; i < contexts.size(); ++i) {
		if (contexts[i] == ctx) {
			return;
		}
	}
	contexts.push_back(ctx);
}

DirectiveRules& DirectiveRules::instance() {
	static DirectiveRules inst;
	return inst;
}

DirectiveRules::DirectiveRules() {
	initializeRules();
}

void DirectiveRules::registerRule(const DirectiveRule& rule) {
	_rules[rule.name] = rule;
}

const DirectiveRule* DirectiveRules::getRule(const std::string& name) const {
	std::map<std::string, DirectiveRule>::const_iterator it = _rules.find(name);
	if (it != _rules.end()) {
		return &it->second;
	}
	return NULL;
}

bool DirectiveRules::hasRule(const std::string& name) const {
	return _rules.find(name) != _rules.end();
}

bool DirectiveRules::isAllowedInContext(const std::string& name, DirectiveContext ctx) const {
	const DirectiveRule* rule = getRule(name);
	if (!rule) return false;
	return rule->isAllowedIn(ctx);
}

void DirectiveRules::initializeRules() {
	// SIZE_MAX used for unlimited arguments

	// --- MAIN context directives ---
	registerRule(DirectiveRule("server", CTX_MAIN, 0, 0, true, true, true));

	// --- SERVER-only directives ---
	registerRule(DirectiveRule("listen", CTX_SERVER, 1, 1, false, true, true));
	registerRule(DirectiveRule("server_name", CTX_SERVER, 1, SIZE_MAX, false, false, true));
	registerRule(DirectiveRule("backlog", CTX_SERVER, 1, 1, false, true, false));
	registerRule(DirectiveRule("keepalive_timeout", CTX_SERVER, 1, 1, false, false, false));
	registerRule(DirectiveRule("keepalive_max_requests", CTX_SERVER, 1, 1, false, false, false));
	registerRule(DirectiveRule("location", CTX_SERVER, 1, 1, true, false, true));

	// --- Common directives (SERVER and LOCATION) ---
	DirectiveRule root("root", CTX_SERVER, 1, 1, false, true, false);
	root.addContext(CTX_LOCATION);
	registerRule(root);

	DirectiveRule index("index", CTX_SERVER, 1, 1, false, false, false);
	index.addContext(CTX_LOCATION);
	registerRule(index);

	DirectiveRule autoindex("autoindex", CTX_SERVER, 1, 1, false, false, false);
	autoindex.addContext(CTX_LOCATION);
	registerRule(autoindex);

	DirectiveRule allowMethods("allow_methods", CTX_SERVER, 1, SIZE_MAX, false, false, false);
	allowMethods.addContext(CTX_LOCATION);
	registerRule(allowMethods);

	DirectiveRule errorPage("error_page", CTX_SERVER, 2, SIZE_MAX, false, false, true);
	errorPage.addContext(CTX_LOCATION);
	registerRule(errorPage);

	DirectiveRule clientMaxBodySize("client_max_body_size", CTX_SERVER, 1, 1, false, true, false);
	clientMaxBodySize.addContext(CTX_LOCATION);
	registerRule(clientMaxBodySize);

	DirectiveRule cgiPath("cgi_path", CTX_SERVER, 1, SIZE_MAX, false, false, false);
	cgiPath.addContext(CTX_LOCATION);
	registerRule(cgiPath);

	DirectiveRule cgiExt("cgi_ext", CTX_SERVER, 1, SIZE_MAX, false, false, false);
	cgiExt.addContext(CTX_LOCATION);
	registerRule(cgiExt);

	// --- LOCATION-only directives ---
	registerRule(DirectiveRule("upload_enabled", CTX_LOCATION, 1, 1, false, false, false));
	registerRule(DirectiveRule("upload_store", CTX_LOCATION, 1, 1, false, false, false));
	registerRule(DirectiveRule("redirect", CTX_LOCATION, 2, 2, false, false, false));
}
