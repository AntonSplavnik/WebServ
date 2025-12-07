#include "../../src/config_v2/config_v2.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return 1;
	}

	try {
		std::cout << "=== Testing New Config Parser V2 ===" << std::endl;
		std::cout << "Config file: " << argv[1] << std::endl << std::endl;

		ConfigV2 config;
		config.parseConfig(argv[1]);

		std::vector<ConfigData> servers = config.getServers();

		std::cout << "SUCCESS! Parsed " << servers.size() << " server(s)" << std::endl << std::endl;

		// Display parsed config
		for (size_t i = 0; i < servers.size(); ++i) {
			std::cout << "--- Server " << (i + 1) << " ---" << std::endl;

			// Listeners
			std::cout << "Listeners:" << std::endl;
			for (size_t j = 0; j < servers[i].listeners.size(); ++j) {
				std::cout << "  " << servers[i].listeners[j].first
				         << ":" << servers[i].listeners[j].second << std::endl;
			}

			// Server names
			if (!servers[i].server_names.empty()) {
				std::cout << "Server names:" << std::endl;
				for (size_t j = 0; j < servers[i].server_names.size(); ++j) {
					std::cout << "  " << servers[i].server_names[j] << std::endl;
				}
			}

			// Root and index
			std::cout << "Root: " << servers[i].root << std::endl;
			std::cout << "Index: " << servers[i].index << std::endl;
			std::cout << "Autoindex: " << (servers[i].autoindex ? "on" : "off") << std::endl;

			// Backlog and body size
			std::cout << "Backlog: " << servers[i].backlog << std::endl;
			std::cout << "Client max body size: " << servers[i].client_max_body_size << std::endl;

			// Keepalive
			std::cout << "Keepalive timeout: " << servers[i].keepalive_timeout << std::endl;
			std::cout << "Keepalive max requests: " << servers[i].keepalive_max_requests << std::endl;

			// Allow methods
			std::cout << "Allowed methods:";
			for (size_t j = 0; j < servers[i].allow_methods.size(); ++j) {
				std::cout << " " << servers[i].allow_methods[j];
			}
			std::cout << std::endl;

			// Locations
			std::cout << "Locations: " << servers[i].locations.size() << std::endl;
			for (size_t j = 0; j < servers[i].locations.size(); ++j) {
				const LocationConfig& loc = servers[i].locations[j];
				std::cout << "  Location: " << loc.path << std::endl;
				std::cout << "    Root: " << loc.root << std::endl;
				std::cout << "    Index: " << loc.index << std::endl;
				std::cout << "    Autoindex: " << (loc.autoindex ? "on" : "off") << std::endl;

				if (loc.upload_enabled) {
					std::cout << "    Upload enabled: " << loc.upload_store << std::endl;
				}

				if (!loc.redirect.empty()) {
					std::cout << "    Redirect: " << loc.redirect_code
					         << " -> " << loc.redirect << std::endl;
				}
			}

			std::cout << std::endl;
		}

		return 0;

	} catch (const std::exception& e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
		return 1;
	}
}
