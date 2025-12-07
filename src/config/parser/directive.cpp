#include "directive.hpp"

Directive::Directive() : name(""), parameters(), children(), line(0), column(0) {}

Directive::Directive(const std::string& n, size_t ln, size_t col)
	: name(n), parameters(), children(), line(ln), column(col) {}

Directive::~Directive() {
	// Clean up all child directives
	for (size_t i = 0; i < children.size(); ++i) {
		delete children[i];
	}
	children.clear();
}

bool Directive::isBlock() const {
	return !children.empty();
}

Directive* Directive::findChild(const std::string& name) const {
	for (size_t i = 0; i < children.size(); ++i) {
		if (children[i]->name == name) {
			return children[i];
		}
	}
	return NULL;
}

std::vector<Directive*> Directive::findChildren(const std::string& name) const {
	std::vector<Directive*> result;
	for (size_t i = 0; i < children.size(); ++i) {
		if (children[i]->name == name) {
			result.push_back(children[i]);
		}
	}
	return result;
}

std::string Directive::getParam(size_t index) const {
	if (index < parameters.size()) {
		return parameters[index];
	}
	return "";
}

bool Directive::hasParam(size_t index) const {
	return index < parameters.size();
}
