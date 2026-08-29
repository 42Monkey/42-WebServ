#include "../include/CGI.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>

CGI::CGI(const Request& req, const LocationConfig& loc, const ServerConfig& srv)
	: _request(req), _location(loc), _server(srv), _pid(-1), _pipeFd(-1)
{
	std::string path = _request.getPath();
	if (path.find("/cgi-bin") == 0)
		path = path.substr(8);
	_scriptPath = _location._root + path;
	_prepareEnv();
	_prepareArgs();
}

CGI::~CGI() {
	if (_pipeFd > 0)
		close(_pipeFd);
}

void CGI::_prepareEnv() {
	_env["REQUEST_METHOD"] = _request.getMethod();
	_env["REQUEST_URI"] = _request.getPath();
	_env["SERVER_PROTOCOL"] = _request.getVersion();

	std::string path = _request.getPath();
	size_t queryPos = path.find('?');
	if (queryPos != std::string::npos) {
		_env["QUERY_STRING"] = path.substr(queryPos + 1);
		_env["SCRIPT_NAME"] = path.substr(0, queryPos);
	} else {
		_env["QUERY_STRING"] = "";
		_env["SCRIPT_NAME"] = path;
	}

	std::ostringstream oss;
	oss << _request.getBody().size();
	_env["CONTENT_LENGTH"] = oss.str();

	const std::map<std::string, std::string>& headers = _request.getHeaders();
	std::map<std::string, std::string>::const_iterator ctIt = headers.find("content-type");
	if (ctIt != headers.end()) {
		_env["CONTENT_TYPE"] = ctIt->second;
	}

	for (std::map<std::string,std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
		std::string key = "HTTP_" + it->first;
		std::transform(key.begin(), key.end(), key.begin(), ::toupper);
		std::replace(key.begin(), key.end(), '-', '_');
		_env[key] = it->second;
	}

	_env["SERVER_NAME"] = _server.getServerName();
	oss.str(""); oss.clear();
	oss << _server.getPort();
	_env["SERVER_PORT"] = oss.str();

	_env["SCRIPT_FILENAME"] = _scriptPath;

	_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	_env["SERVER_SOFTWARE"] = "WebServ/1.0";

	if (_getExtension() == ".php") {
		_env["REDIRECT_STATUS"] = "200";
	}

	_env["PATH"] = "/usr/bin:/bin:/usr/local/bin";
	if (_getExtension() == ".py") {
		_env["PYTHONPATH"] = "/usr/lib/python3:/usr/local/lib/python3";
	}
}

void CGI::_prepareArgs() {
	std::string interp = _getInterpreter();
	if (!interp.empty())
		_args.push_back(interp);
	_args.push_back(_scriptPath);
}

std::string CGI::_getExtension() const {
	size_t dot = _scriptPath.rfind('.');
	if (dot == std::string::npos) return "";
	return _scriptPath.substr(dot);
}

std::string CGI::_getInterpreter() const {
	for (size_t i = 0; i < _location._cgiExtensions.size(); ++i) {
		if (_location._cgiExtensions[i] == _getExtension())
			return _location._cgiInterpreters[i];
	}
	return "";
}

// // checks if script exist
// // checks if a regular file
// bool CGI::_validateScript() const {
// 	struct stat script_stat;
// 	if (stat(_scriptPath.c_str(), &script_stat) != 0) {
// 		return false;
// 	}

// 	if (!S_ISREG(script_stat.st_mode)) {
// 		return false;
// 	}

// 	return true;
// }

// // no interpreter needed
// // checks if interpreter exists
// // check if interpreter is executable
// bool CGI::_validateInterpreter() const {
// 	std::string interpreter = _getInterpreter();
// 	if (interpreter.empty()) {
// 		return true;
// 	}

// 	struct stat interp_stat;
// 	if (stat(interpreter.c_str(), &interp_stat) != 0) {
// 		return false;
// 	}

// 	if (!(interp_stat.st_mode & S_IXUSR)) {
// 		return false;
// 	}

// 	return true;
// }

bool CGI::_setupPipes(int pipe_out[2], int pipe_in[2]) const {
	if (pipe(pipe_out) < 0 || pipe(pipe_in) < 0) {
		return false;
	}

	int flags = fcntl(pipe_out[0], F_GETFL, 0);
	if (flags >= 0) {
		fcntl(pipe_out[0], F_SETFL, flags | O_NONBLOCK);
	}

	return true;
}

void CGI::_setupChildProcess(int pipe_out[2], int pipe_in[2]) const {
	close(pipe_out[0]);
	close(pipe_in[1]);

	dup2(pipe_out[1], STDOUT_FILENO);
	dup2(pipe_out[1], STDERR_FILENO);
	dup2(pipe_in[0], STDIN_FILENO);

	close(pipe_out[1]);
	close(pipe_in[0]);
}

// prepare argv
// prepare envp
// if fail, exit silently
void CGI::_executeScript() const {

	std::vector<char*> argv;
	for (size_t i = 0; i < _args.size(); ++i) {
		argv.push_back(const_cast<char*>(_args[i].c_str()));
	}
	argv.push_back(NULL);


	std::vector<std::string> envStrs;
	std::vector<char*> envp;
	for (std::map<std::string,std::string>::const_iterator it = _env.begin(); it != _env.end(); ++it) {
		envStrs.push_back(it->first + "=" + it->second);
	}
	for (size_t i = 0; i < envStrs.size(); ++i) {
		envp.push_back(const_cast<char*>(envStrs[i].c_str()));
	}
	envp.push_back(NULL);

	execve(argv[0], &argv[0], &envp[0]);
	// If we reach here, execve failed
	_exit(1);
}

void CGI::_handlePostData(int pipe_in_fd) {
	if (_request.getMethod() != "POST") {
		return;
	}

	std::string body = _request.getBody();
	const char* data = body.c_str();
	size_t remaining = body.size();

	while (remaining > 0) {
		ssize_t n = write(pipe_in_fd, data, remaining);
		if (n <= 0) {
			break;
		}
		data += n;
		remaining -= n;
	}
}

// validates script and interpreter before forking
int CGI::executeAsync(int client_fd) {
	_clientFd = client_fd;

	// if (!_validateScript()) {
	// 	return -1;
	// }

	// if (!_validateInterpreter()) {
	// 	return -1;
	// }

	int pipe_out[2], pipe_in[2];
	if (!_setupPipes(pipe_out, pipe_in)) {
		return -1;
	}

	pid_t pid = fork();
	if (pid < 0) {
		close(pipe_out[0]); close(pipe_out[1]);
		close(pipe_in[0]);  close(pipe_in[1]);
		return -1;
	}

	if (pid == 0) {
		_setupChildProcess(pipe_out, pipe_in);
		_executeScript();
	}

	close(pipe_out[1]);
	close(pipe_in[0]);
	_pid = pid;
	_pipeFd = pipe_out[0];

	_handlePostData(pipe_in[1]);
	close(pipe_in[1]);

	return _pipeFd;
}
