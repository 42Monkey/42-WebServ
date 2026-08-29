#include "../include/Router.hpp"
#include "../include/ServerConfig.hpp"
#include "../include/LocationConfig.hpp"
#include "../include/Request.hpp"

Router::Router(const std::vector<ServerConfig>& configs) : _configs(configs) {}

Router::~Router() {}

const ServerConfig* Router::getServerConfig(const std::string &host, int port) const {
	if (_configs.empty()) {
		return NULL;
	}

	std::string hostname = host;

	std::string::size_type colonPosition = hostname.find(':');
	if (colonPosition != std::string::npos) {
		hostname = hostname.substr(0, colonPosition);
	}

	const ServerConfig* portFallback = NULL;

	for (size_t i = 0; i < _configs.size(); ++i) {
		const ServerConfig& config = _configs[i];

		const std::vector<std::string>& serverNames = config.getServerNames();
		for (size_t j = 0; j < serverNames.size(); ++j) {
			if (serverNames[j] == hostname) {
				return &config;
			}
		}

		const std::vector<int>& ports = config._listenPorts;
		for (size_t j = 0; j < ports.size(); ++j) {
			if (ports[j] == port) {
				portFallback = &config;
				break ;
			}
		}
	}

	if (portFallback != NULL) {
		return portFallback;
	}
	return &_configs[0];
}

const LocationConfig* Router::getLocationConfig(const ServerConfig* server, const std::string& path) const {
	if (!server) {
		return NULL;
	}

	const std::vector<LocationConfig>& locations = server->getLocations();
	const LocationConfig* bestMatch = NULL;
	size_t bestMatchLength = 0;

	for (size_t i = 0; i < locations.size(); ++i) {
		const LocationConfig& location = locations[i];
		const std::string& locationPath = location._path;

		if (path.compare(0, locationPath.length(), locationPath) == 0) {
			if (locationPath.length() > bestMatchLength) {
				bestMatch = &location;
				bestMatchLength = locationPath.length();
			}
		}
	}
	return bestMatch;
}

const LocationConfig* Router::match(const ServerConfig*& outServer,
	const Request& req,
	int port) const {
	outServer = getServerConfig(req.getHeader("Host"), port);
	if (!outServer)
	return NULL;

	return getLocationConfig(outServer, req.getPath());
}
