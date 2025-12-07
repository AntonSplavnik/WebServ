#include "config.hpp"
#include "exceptions/config_exceptions.hpp"
#include <fstream>
#include <sstream>

Config::Config() : _servers(), _tree() {}

Config::~Config() {
	cleanup();
}

void Config::cleanup() {
	for (size_t i = 0; i < _tree.size(); ++i) {
		delete _tree[i];
	}
	_tree.clear();
}

std::string Config::readFile(const std::string& filepath) {
	std::ifstream file(filepath.c_str());
	if (!file.is_open()) {
		throw ConfigException("Failed to open config file: " + filepath, 0, 0);
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

bool Config::parseConfig(const char* filepath) {
	return parseConfig(const_cast<char*>(filepath));
}

bool Config::parseConfig(char* filepath) {
	std::string configPath = std::string(filepath);

	try {
		cleanup(); // Clean up any previous parse

		// Step 1: Read file
		std::string content = readFile(configPath);

		// Step 2: Lexer - tokenize
		Lexer lexer(content, configPath);
		std::vector<Token> tokens = lexer.tokenize();

		// Step 3: Parser - build directive tree
		Parser parser(tokens, configPath);
		_tree = parser.parse();

		// Step 4: Validator - semantic checks
		Validator validator(configPath);
		validator.validate(_tree);

		// Step 5: Builder - convert to ConfigData objects
		ConfigBuilder builder;
		_servers = builder.build(_tree);

		return true;

	} catch (const std::exception& e) {
		// Just re-throw - exceptions already have good error messages
		throw;
	}
}

std::vector<ConfigData> Config::getServers() const {
	return _servers;
}
