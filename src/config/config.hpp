#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "validator/validator.hpp"
#include "builder/config_builder.hpp"
#include "config_structures.hpp" // For ConfigData and LocationConfig
#include <string>
#include <vector>

// New nginx-style config parser
class Config {
public:
	Config();
	~Config();

	// Parse config file (same interface as old parser)
	bool parseConfig(char* filepath);
	bool parseConfig(const char* filepath);

	// Get parsed servers
	std::vector<ConfigData> getServers() const;

private:
	std::vector<ConfigData> _servers;
	std::vector<Directive*> _tree; // Keep tree for cleanup

	// Helper methods
	std::string readFile(const std::string& filepath);
	void cleanup();
};

#endif // CONFIG_HPP
