#ifndef CONFIG_BUILDER_HPP
#define CONFIG_BUILDER_HPP

#include "../parser/directive.hpp"
#include "../config_structures.hpp" // For ConfigData and LocationConfig
#include <vector>
#include <string>

class ConfigBuilder {
public:
	ConfigBuilder();

	// Build ConfigData objects from validated directive tree
	std::vector<ConfigData> build(const std::vector<Directive*>& tree);

private:
	// Build a single server config
	ConfigData buildServer(const Directive* server);

	// Build a single location config
	LocationConfig buildLocation(const Directive* location);

	// Apply directive to server config
	void applyServerDirective(ConfigData& config, const Directive* dir);

	// Apply directive to location config
	void applyLocationDirective(LocationConfig& config, const Directive* dir);

	// Specific directive handlers for server
	void handleListen(ConfigData& config, const Directive* dir);
	void handleServerName(ConfigData& config, const Directive* dir);
	void handleBacklog(ConfigData& config, const Directive* dir);
	void handleKeepaliveTimeout(ConfigData& config, const Directive* dir);
	void handleKeepaliveMaxRequests(ConfigData& config, const Directive* dir);

	// Common directive handlers (work for both server and location)
	template<typename ConfigT>
	void handleRoot(ConfigT& config, const Directive* dir);

	template<typename ConfigT>
	void handleIndex(ConfigT& config, const Directive* dir);

	template<typename ConfigT>
	void handleAutoindex(ConfigT& config, const Directive* dir);

	template<typename ConfigT>
	void handleAllowMethods(ConfigT& config, const Directive* dir);

	template<typename ConfigT>
	void handleErrorPage(ConfigT& config, const Directive* dir);

	template<typename ConfigT>
	void handleClientMaxBodySize(ConfigT& config, const Directive* dir);

	template<typename ConfigT>
	void handleCgiPath(ConfigT& config, const Directive* dir);

	template<typename ConfigT>
	void handleCgiExt(ConfigT& config, const Directive* dir);

	// Location-specific handlers
	void handleUploadEnabled(LocationConfig& config, const Directive* dir);
	void handleUploadStore(LocationConfig& config, const Directive* dir);
	void handleRedirect(LocationConfig& config, const Directive* dir);
};

#include "config_builder.tpp" // Template implementations

#endif // CONFIG_BUILDER_HPP
