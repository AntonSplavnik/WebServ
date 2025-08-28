#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <cstdint>
#include <map>

struct ConfigData{
	std::string host;
	uint16_t    port;
	std::string server_name;
	std::string root;
	std::string index;
	int         backlog;
	std::map <int, std::string> error_pages;
	std::string access_log;
	std::string error_log;
	std::vector<std::string> cgi_path;
	std::vector<std::string> cgi_ex