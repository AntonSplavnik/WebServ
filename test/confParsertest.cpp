
#include "../src/config/config.hpp"
#include "../src/exceptions/config_exceptions.hpp"
#include <iostream>
#include <string>
#include "../src/logging/logger.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Wrong config path" << std::endl;
        return 1;
    }

    Config config;
    try {
        config.parseConfig(argv);
    } catch (const ConfigParseException& e) {
        std::cerr << "Error parsing config file: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Config file loaded successfully" << std::endl;

    const std::vector<ConfigData>& servers = config.getServers();
    if (servers.empty()) {
        std::cout << "No server blocks found in config." << std::endl;
        return 1;
    }

    for (size_t s = 0; s < servers.size(); ++s) {
        const ConfigData& data = servers[s];
        std::cout << "=== Server block " << s << " ===" << std::endl;
        for (size_t i = 0; i < data.listeners.size(); i++) {
            std::cout << data.listeners[i].first << ":" << data.listeners[i].second << std::endl;
        }
        for (size_t i = 0; i < data.server_names.size(); i++) {
            std::cout << data.server_names[i].c_str() << std::endl;
        }
        std::cout << data.root << std::endl;
        std::cout << data.index << std::endl;
        std::cout << data.backlog << std::endl;
        const std::map<int, std::string>& errorPages = data.error_pages;
        for (std::map<int, std::string>::const_iterator it = errorPages.begin(); it != errorPages.end(); it++) {
            std::cout << it->first << " => " << it->second << std::endl;
        }
        for (size_t i = 0; i < data.allow_methods.size(); i++) {
            std::cout << data.allow_methods[i] << " ";
        }
        std::cout << std::endl;
        std::cout << data.access_log << std::endl;
        std::cout << data.error_log << std::endl;
        std::cout << std::endl;
        for (size_t i = 0; i < data.cgi_ext.size(); i++) {
            std::cout << data.cgi_ext[i] << " ";
        }
        std::cout << std::endl;
        for (std::vector<std::string>::const_iterator it = data.cgi_path.begin(); it != data.cgi_path.end(); ++it) {
            std::cout << *it << " ";
        }
        std::cout << std::endl;
        std::cout << (data.autoindex ? "Autoindex is enabled" : "Autoindex is disabled") << std::endl;
        std::cout << "Locations:" << std::endl;
        std::cout << "Number of locations: " << data.locations.size() << std::endl;
        std::cout << "Client max body size: " << data.client_max_body_size << std::endl;
        for (size_t i = 0; i < data.locations.size(); i++) {
            const LocationConfig& loc = data.locations[i];
            std::cout << "  Path: " << loc.path << std::endl;
            std::cout << "  Root: " << loc.root << std::endl;
            std::cout << "  Index: " << loc.index << std::endl;
            std::cout << "  Autoindex: " << (loc.autoindex ? "on" : "off") << std::endl;
            std::cout << "  Allowed Methods: ";
            for (size_t j = 0; j < loc.allow_methods.size(); j++) {
                std::cout << loc.allow_methods[j] << " ";
            }
            std::cout << std::endl;
            std::cout << "  Error Pages: ";
            for (std::map<int, std::string>::const_iterator it = loc.error_pages.begin(); it != loc.error_pages.end(); ++it) {
                std::cout << it->first << "=>" << it->second << " ";
            }
            std::cout << std::endl << std::endl;
            for (size_t j = 0; j < loc.cgi_path.size(); j++) {
                std::cout << loc.cgi_path[j] << " ";
            }
            std::cout << std::endl;
            for (size_t j = 0; j < loc.cgi_ext.size(); j++) {
                std::cout << loc.cgi_ext[j] << " ";
            }
            std::cout << std::endl;

            std::cout << "client max body size: " << loc.client_max_body_size << std::endl;
            std::cout << "upload enabled: " << (loc.upload_enabled ? "yes" : "no") << std::endl;
            std::cout << "upload store: " << loc.upload_store << std::endl;
            std::cout << "redirect: " << loc.redirect << std::endl;
            std::cout << "redirect code: " << loc.redirect_code << std::endl;
            std::cout << "------------------------" << std::endl;
        }

        // test logging:
        Logger logger(data.access_log, data.error_log);
        logger.logAccess("127.0.0.1", "GET", "/index.html", 200, 512);
        logger.logError("ERROR", "Failed to open configuration file.");
        std::cout << "Logging test complete. Check access.log and error.log files." << std::endl;
    }
    return 0;
}
