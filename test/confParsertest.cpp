#include "../src/config/config.hpp"
#include <iostream>
#include <string>

// c++ -std=c++98 confParsertest.cpp ../src/config/config.cpp
int main(int argc, char* argv[]) {
    std::string configPath = "../src/config/default.conf";
    if (argc > 1)
    {
        configPath = "../src/config/" + std::string(argv[1]);
    }

    Config config;
    if (!config.parseConfig(configPath)) {
        std::cerr << "Failed to load config file: " << configPath << std::endl;
        return 1;
    }
    std::cout << "Config file loaded successfully from: " << configPath << std::endl;
    // Use config.getConfigData() as needed
    std::cout << config.getConfigData().host << std::endl;
    std::cout << config.getConfigData().port << std::endl;
    std::cout << config.getConfigData().server_name << std::endl;
    std::cout << config.getConfigData().root << std::endl;
    std::cout << config.getConfigData().index << std::endl;
    std::cout << config.getConfigData().backlog << std::endl;
    const std::map<int, std::string>& errorPages = config.getConfigData().error_pages;
    for (std::map<int, std::string>::const_iterator it = errorPages.begin(); it != errorPages.end(); it++) {
        std::cout << it->first << " => " << it->second << std::endl;
    }
    return 0;
}