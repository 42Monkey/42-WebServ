#include "../include/ServerConfig.hpp"
#include "../include/LocationConfig.hpp"
#include "../include/Socket.hpp"

ServerConfig::ServerConfig()
	: _listenPorts(),
	  _clientMaxBodySize(1 * 1024 * 1024),
	  _locations() {}

ServerConfig::ServerConfig(const ServerConfig &source)
	: _listenPorts(source._listenPorts),
	  _host(source._host),
	  _serverNames(source._serverNames),
	  _errorPages(source._errorPages),
	  _clientMaxBodySize(source._clientMaxBodySize),
	  _locations(source._locations) {}

ServerConfig& ServerConfig::operator=(const ServerConfig &rhs) {
	if (this != &rhs) {
		_listenPorts = rhs._listenPorts;
		_host = rhs._host;
		_serverNames = rhs._serverNames;
		_errorPages = rhs._errorPages;
		_clientMaxBodySize = rhs._clientMaxBodySize;
		_locations = rhs._locations;
	}
	return *this;
}

ServerConfig::~ServerConfig() {}

// Getters
std::string ServerConfig::getHost() const {
	return _host;
}

int ServerConfig::getPort() const {
	if (_listenPorts.empty())
		throw std::runtime_error("No listen port configured in ServerConfig");
	return _listenPorts[0];
}

std::string ServerConfig::getServerName() const {
	if (_serverNames.empty()) {
		return "";
	} else {
		return _serverNames[0];
	}
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const {
	return _errorPages;
}

const std::vector<LocationConfig>& ServerConfig::getLocations() const {
	return _locations;
}

const std::vector<std::string>& ServerConfig::getServerNames() const {
	return _serverNames;
}

// Within ServerConfig.hpp or ServerConfig.cpp
int ServerConfig::createListeningSocket() const {
	Socket sock;

	// Check socket creation
	if (!sock.createSocket()) {
		return -1;
	}

	// Check non-blocking mode
	if (!sock.setNonBlocking()) {
		return -1;
	}

	// Check socket binding
	if (!sock.bindSocket(getHost(), getPort())) {
		return -1;
	}

	// Check socket listening
	if (!sock.listenSocket(1024)) {
		return -1;
	}

	return sock.getFd();
}
