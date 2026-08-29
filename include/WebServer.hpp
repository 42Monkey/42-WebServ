#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "ServerConfig.hpp"
#include "Socket.hpp"
#include <vector>
#include <string>
#include <map>
#include <ctime>
#include <dirent.h>
#include <iomanip>

class Multiplexer;
class Router;
class Request;
class Response;
class CGI;

class WebServer {
private:
	std::vector<ServerConfig>	_configs;
	Multiplexer*				_multiplexer;
	Router*						_router;

	// Keep sockets alive
	std::vector<Socket*>		_serverSockets;
	std::map<int, size_t>		_serverFdToConfig;
	std::map<int, std::string>	_errorPage;

	// HTTP processing methods
	void _handlePost(const Request& request, const LocationConfig& location, const ServerConfig& server);
	void _handleGet(const Request& request, const LocationConfig& location, const ServerConfig& server);
	void _handleDelete(const Request& request, const LocationConfig& location, const ServerConfig& server);
	void _handleCgi(int client_fd, const Request& request, const LocationConfig& location, const ServerConfig& server);
	bool _isAsyncFileOperation(const std::string& path) const;
	bool _handleRedirect(int client_fd, const LocationConfig& location);
	bool _isCgiRequest(const std::string& path, const LocationConfig& location);

	size_t getConfigIndexFromClient(int client_fd) const;;

	// File Handling
	bool _checkDirectory(int client_fd, const std::string& path);
	std::string _extractFilename(const Request& request);
	std::string _resolveFilename(const std::string& uploadDir, const std::string& originalFilename);
	std::string _formatTimestamp(time_t rawTime);

	// Directory listing methods
	std::string _formatFileSize(off_t size);
	std::string _buildEntryLink(const std::string& requestPath, const std::string& name, bool isDirectory);
	std::string _generateDirectoryHTML(const std::string& path, const std::string& requestPath);
	bool _handleDirectory(int client_fd, const std::string& directoryPath, const std::string& requestPath, const LocationConfig& location);

	// Response operations
	void _sendResponse(int client_fd, const Response& response);

	// Disable copying
	WebServer(const WebServer &source);
	WebServer& operator=(const WebServer &rhs);

public:
	WebServer(const std::vector<ServerConfig>& configs);
	~WebServer();

	bool initialize();
	void run();

	void processRequest(int client_fd, const std::string& raw);
	size_t getConfigIndex(int server_fd) const;
	size_t	_getMaxBodySizeForClient(int cliend_fd) const;

	bool handleTraversal(const std::string& root, const std::string& user, std::string& path);

	void _initErrorPage();
	std::string _generateErrorPage(int code, const std::string& message);
	void _sendError(int client_fd, int code, const std::string& message);
};

#endif
