#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include <vector>
#include <map>
#include "../include/Request.hpp"
#include "../include/LocationConfig.hpp"
#include "../include/ServerConfig.hpp"

class CGI {
public:
	CGI(const Request& req, const LocationConfig& loc, const ServerConfig& srv);
	~CGI();

	int executeAsync(int client_fd);

	int getPipeFd() const { return _pipeFd; }
	pid_t getPid() const { return _pid; }

private:
	const Request& _request;
	const LocationConfig& _location;
	const ServerConfig& _server;
	std::string _scriptPath;

	pid_t _pid;
	int _pipeFd;
	int _clientFd;

	std::map<std::string, std::string> _env;
	std::vector<std::string> _args;

	void _prepareEnv();
	void _prepareArgs();

	// bool _validateScript() const;
	// bool _validateInterpreter() const;
	bool _setupPipes(int pipe_out[2], int pipe_in[2]) const;
	void _setupChildProcess(int pipe_out[2], int pipe_in[2]) const;
	void _executeScript() const;
	void _handlePostData(int pipe_in_fd);

	std::string _getExtension() const;
	std::string _getInterpreter() const;

};

#endif
