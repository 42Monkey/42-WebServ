#ifndef ROUTER_HPP
# define ROUTER_HPP

# include "ServerConfig.hpp"
# include "LocationConfig.hpp"
# include "Request.hpp"
# include <vector>
# include <string>
# include <map>
# include <iostream>
# include <sstream>
# include <cstdlib>
# include <string>

class Router {
	private :

		const std::vector<ServerConfig>&	_configs;

		Router(const Router &source);
		Router& operator=(const Router &rhs);

	public :
		Router(const std::vector<ServerConfig>& configs);
		~Router();

		const ServerConfig* getServerConfig(const std::string& host, int port) const;
		const LocationConfig* getLocationConfig(const ServerConfig* server, const std::string& path) const;
		const LocationConfig* match(const ServerConfig*& outServer, const Request& req, int port) const;
};

#endif
