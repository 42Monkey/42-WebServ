#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

# include <string>
# include <vector>
# include <map>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <cstring>
# include <iostream>
# include <stdexcept>

class LocationConfig;

class ServerConfig {

	public:
	std::vector<int>			_listenPorts;
	std::string					_host;
	std::vector<std::string>	_serverNames;
	std::map<int, std::string>	_errorPages;
	size_t						_clientMaxBodySize;
	std::vector<LocationConfig>	_locations;

	ServerConfig();
	ServerConfig(const ServerConfig &source);
	ServerConfig &operator=(const ServerConfig &rhs);
	~ServerConfig();

	// Getters
	std::string getHost() const;
	int getPort() const;

	// const std::vector<int>& getPort() const;
	// const std::string& getHost() const;
	std::string getServerName() const;
	const std::map<int, std::string>& getErrorPages() const;
	const std::vector<LocationConfig>& getLocations() const;
	const std::vector<std::string>& getServerNames() const;
	size_t getClientMaxBodySize() const;

	int createListeningSocket() const;
};

#endif
