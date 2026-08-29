#include "../include/Socket.hpp"
#include "../include/Request.hpp"
#include "../include/Multiplexer.hpp"
#include "../include/WebServer.hpp"
#include "../include/Response.hpp"
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <queue>

Multiplexer::Multiplexer(WebServer* server) : _webServer(server), _running(false) {
	_epollFd = epoll_create(1);
	if (_epollFd == -1) {
		std::cerr << "epoll_create failed" << std::endl;
	} else {
		// CLOEXEC
		int flags = fcntl(_epollFd, F_GETFD);
		if (flags != -1) {
			fcntl(_epollFd, F_SETFD, flags | FD_CLOEXEC);
		}
	}
	struct sigaction sa;
	sa.sa_handler = _sigchldHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
	}
}

Multiplexer::~Multiplexer() {
	for (std::map<int, Socket*>::iterator it = _clientSockets.begin(); it != _clientSockets.end(); ++it) {
		::close(it->first);
		delete it->second;
	}
	
	// Clean up file operations
	for (std::map<int, FileOperation>::iterator it = _fileOperations.begin(); it != _fileOperations.end(); ++it) {
		epoll_ctl(_epollFd, EPOLL_CTL_DEL, it->first, NULL);
		close(it->first);
	}
	_fileOperations.clear();

    if (_epollFd != -1) {
        ::close(_epollFd);
    }
}

bool Multiplexer::startFileRead(int client_fd, const std::string& file_path) {
    int file_fd = open(file_path.c_str(), O_RDONLY);
    if (file_fd < 0) {
        return false;
    }

    // Get file size
    struct stat file_stat;
    if (fstat(file_fd, &file_stat) != 0) {
        close(file_fd);
        return false;
    }

    // Read whole file into memory
    std::ostringstream content;
    char buffer[8192];
    ssize_t n;
    while ((n = read(file_fd, buffer, sizeof(buffer))) > 0) {
        content.write(buffer, n);
    }
    close(file_fd);

    if (n < 0) return false;

    // Send headers
    if (!_sendFileHeaders(client_fd, file_path, content.str().size())) {
        return false;
    }

    // Queue body for writing
    _writeBuffers[client_fd] += content.str();

    // Enable EPOLLOUT so we can flush to client
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = client_fd;
    epoll_ctl(_epollFd, EPOLL_CTL_MOD, client_fd, &ev);

    return true;
}

bool Multiplexer::startFileWrite(int client_fd, const std::string& file_path, const std::string& data) {
    int file_fd = open(file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_fd < 0) {
        perror("open file for write");
        _webServer->_sendError(client_fd, 500, "Failed to open file");
        return false;
    }

    ssize_t written = 0;
    size_t total = data.size();
    const char* ptr = data.data();

    while (written < (ssize_t)total) {
        ssize_t n = write(file_fd, ptr + written, total - written);
        if (n < 0) {
            perror("write file");
            close(file_fd);
            _webServer->_sendError(client_fd, 500, "Failed to save file");
            return false;
        }
        written += n;
    }

    close(file_fd);

    // Send success response
    std::ostringstream response;
    response << "HTTP/1.1 201 Created\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: ";
    std::string body = "<h1>File uploaded successfully</h1>";
    response << body.size() << "\r\n";
    response << "\r\n";
    response << body;

    queueResponse(client_fd, response.str());
    return true;
}

bool Multiplexer::_sendFileHeaders(int client_fd, const std::string& file_path, size_t file_size) {
	std::ostringstream headers;
	headers << "HTTP/1.1 200 OK\r\n";
	headers << "Content-Type: " << _getContentType(file_path) << "\r\n";
	headers << "Content-Length: " << file_size << "\r\n";
	headers << "\r\n";
	
	_writeBuffers[client_fd] += headers.str();
	
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLOUT;
	ev.data.fd = client_fd;
	epoll_ctl(_epollFd, EPOLL_CTL_MOD, client_fd, &ev);
	
	return true;
}

std::string Multiplexer::_getContentType(const std::string& file_path) {
	size_t dot = file_path.find_last_of('.');
	if (dot == std::string::npos) {
		return "application/octet-stream";
	}
	
	std::string ext = file_path.substr(dot + 1);
	if (ext == "html" || ext == "htm") return "text/html";
	if (ext == "css") return "text/css";
	if (ext == "js") return "application/javascript";
	if (ext == "json") return "application/json";
	if (ext == "xml") return "application/xml";
	if (ext == "txt") return "text/plain";
	if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
	if (ext == "png") return "image/png";
	if (ext == "gif") return "image/gif";
	if (ext == "pdf") return "application/pdf";
	
	return "application/octet-stream";
}

void Multiplexer::_handleFileRead(int file_fd) {
	FileOperation& fileOp = _fileOperations[file_fd];
	char buffer[8192];
	ssize_t bytes_read;
	
	while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
		_writeBuffers[fileOp.client_fd].append(buffer, bytes_read);
		fileOp.bytes_processed += bytes_read;
		
		// Ensure client socket is monitored for write
		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLOUT;
		ev.data.fd = fileOp.client_fd;
		epoll_ctl(_epollFd, EPOLL_CTL_MOD, fileOp.client_fd, &ev);
	}
	
	if (bytes_read == 0) {
		// EOF reached
		_closeFileOperation(file_fd);
	} else if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
		// Error occurred
		perror("read file");
		_closeFileOperation(file_fd);
		_webServer->_sendError(fileOp.client_fd, 500, "Internal Server Error");
	}
}

void Multiplexer::_handleFileWrite(int file_fd) {
	FileOperation& fileOp = _fileOperations[file_fd];
	ssize_t bytes_written;
	
	while (fileOp.bytes_processed < fileOp.data.size()) {
		bytes_written = write(file_fd, 
			fileOp.data.data() + fileOp.bytes_processed,
			fileOp.data.size() - fileOp.bytes_processed);
		
		if (bytes_written > 0) {
			fileOp.bytes_processed += bytes_written;
		} else if (bytes_written < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return; // Try again later
			} else {
				// Error occurred
				perror("write file");
				_closeFileOperation(file_fd);
				_webServer->_sendError(fileOp.client_fd, 500, "Failed to save file");
				return;
			}
		}
	}
	
	// All data written successfully
	_closeFileOperation(file_fd);
	
	// Send success response
	std::ostringstream response;
	response << "HTTP/1.1 201 Created\r\n";
	response << "Content-Type: text/html\r\n";
	response << "Content-Length: ";
	
	std::string body = "<h1>File uploaded successfully</h1>";
	response << body.size() << "\r\n";
	response << "\r\n";
	response << body;
	
	_writeBuffers[fileOp.client_fd] += response.str();
	
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLOUT;
	ev.data.fd = fileOp.client_fd;
	epoll_ctl(_epollFd, EPOLL_CTL_MOD, fileOp.client_fd, &ev);
}

void Multiplexer::_closeFileOperation(int file_fd) {
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, file_fd, NULL);
	close(file_fd);
	_fileOperations.erase(file_fd);
}

bool Multiplexer::_isFileOperation(int fd) const {
	return _fileOperations.count(fd) > 0;
}

bool Multiplexer::addServerSocket(int server_fd) {
	if (_epollFd == -1) {
		std::cerr << "Epoll not initialized" << std::endl;
		return false;
	}

	if (fcntl(server_fd, F_GETFD) == -1) return false;
	if (fcntl(_epollFd, F_GETFD) == -1) return false;

	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = server_fd;

	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, server_fd, &event) == -1) return false;

	_serverFds.push_back(server_fd);
	return true;
}

void Multiplexer::run() {
	const int INITIAL_MAX_EVENTS = 64;
	int max_events = INITIAL_MAX_EVENTS;
	struct epoll_event* events = new epoll_event[max_events];

	if (_epollFd == -1) {
		std::cerr << "Cannot run: epoll not initialized" << std::endl;
		delete[] events;
		return;
	}

	_running = true;

	// add stdin listen server command
	struct epoll_event stdin_event;
	stdin_event.events = EPOLLIN;
	stdin_event.data.fd = STDIN_FILENO;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, STDIN_FILENO, &stdin_event) == -1) {
		std::cerr << "Failed to add stdin to epoll: " << strerror(errno) << std::endl;
	}

	std::cout << "Multiplexer started, waiting for events..." << std::endl;

	while (_running) {
		int ready = epoll_wait(_epollFd, events, max_events, -1);
		if (ready == -1) {
			if (errno == EINTR) continue;
			std::cerr << "epoll_wait failed: " << strerror(errno) << std::endl;
			break;
		}
		processSigchldQueue();
		// if ready == max_events，expand
		if (ready == max_events) {
			max_events *= 2;
			epoll_event* new_events = new epoll_event[max_events];
			memcpy(new_events, events, ready * sizeof(epoll_event));
			delete[] events;
			events = new_events;
		}

		for (int i = 0; i < ready; ++i) {
			int fd = events[i].data.fd;

			if (fd == STDIN_FILENO) {
				std::string command;
				if (std::getline(std::cin, command)) {
					if (command == "exit") _running = false;
					else if (command == "status")
						std::cout << "Active connections: " << _clientSockets.size() << std::endl;
				}
			} else if (_isServerSocket(fd)) {
				_handleNewConnection(fd);
			} else if (_cgiPipes.find(fd) != _cgiPipes.end()) {
				_handleCgiPipeRead(fd);
			} else if (_isFileOperation(fd)) {
				if (events[i].events & EPOLLIN) _handleFileRead(fd);
				if (events[i].events & EPOLLOUT) _handleFileWrite(fd);
			} else {
				if (events[i].events & EPOLLIN) _handleClientRead(fd);
				if (events[i].events & EPOLLOUT) _handleClientWrite(fd);
			}
		}
	}

	delete[] events;
}

// Rest of the existing methods remain unchanged...

bool Multiplexer::_isServerSocket(int fd) const {
	for (size_t i = 0; i < _serverFds.size(); ++i) {
		if (_serverFds[i] == fd) return true;
	}
	return false;
}

void Multiplexer::addCgi(int client_fd, int pipe_fd, CGI* cgi) {
	_cgiPipes[pipe_fd] = std::make_pair(client_fd, cgi);
	_cgiHeadersSent[pipe_fd] = false;

	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET; // edge-triggered
	ev.data.fd = pipe_fd;

	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, pipe_fd, &ev) == -1) {
		perror("epoll_ctl: add CGI pipe");
		close(pipe_fd);
		delete cgi;
		_cgiPipes.erase(pipe_fd);
	}
}

void Multiplexer::_handleCgiPipeRead(int pipe_fd) {
	char tmp[4096];
	ssize_t n;

	std::pair<int, CGI*>& info = _cgiPipes[pipe_fd];
	int client_fd = info.first;
	CGI* cgi = info.second;

	std::string& buf = _writeBuffers[client_fd];
	bool& headersSent = _cgiHeadersSent[pipe_fd]; // track per-pipe

	while ((n = read(pipe_fd, tmp, sizeof(tmp))) > 0) {
		buf.append(tmp, n);

		if (!headersSent) {
			size_t pos = buf.find("\r\n\r\n");
			if (pos == std::string::npos)
				pos = buf.find("\n\n");
			if (pos != std::string::npos) {
				std::string headers = buf.substr(0, pos + (buf[pos] == '\r' ? 4 : 2));
				std::string body = buf.substr(pos + (buf[pos] == '\r' ? 4 : 2));

				if (headers.find("HTTP/") != 0) {
					headers = "HTTP/1.1 200 OK\r\n" + headers;
				}

				if (headers.find("Content-Length:") == std::string::npos) {
					std::ostringstream oss;
					oss << "Content-Length: " << body.size() << "\r\n";
					headers += oss.str();
				}
				headers += "\r\n";
				buf = headers;
				buf += body;
				headersSent = true;
			}
		}

	}

	if (n == 0) {
		// CGI finished
		epoll_ctl(_epollFd, EPOLL_CTL_DEL, pipe_fd, 0);
		close(pipe_fd);

		// Ensure client socket monitored for write
		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLOUT;
		ev.data.fd = client_fd;
		epoll_ctl(_epollFd, EPOLL_CTL_MOD, client_fd, &ev);

		delete cgi;
		_cgiPipes.erase(pipe_fd);
		_cgiHeadersSent.erase(pipe_fd);
	} else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
		perror("read CGI pipe");
		epoll_ctl(_epollFd, EPOLL_CTL_DEL, pipe_fd, 0);
		close(pipe_fd);
		delete cgi;
		_cgiPipes.erase(pipe_fd);
		_cgiHeadersSent.erase(pipe_fd);
	}
}

void Multiplexer::_handleNewConnection(int server_fd) {
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);
	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
	if (client_fd == -1) return;

	int flags = fcntl(client_fd, F_GETFL, 0);
	if (flags >= 0) fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

	Socket* clientSocket = new Socket(client_fd);
	_clientSockets[client_fd] = clientSocket;
	_clientToServerFd[client_fd] = server_fd;

	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = client_fd;
	epoll_ctl(_epollFd, EPOLL_CTL_ADD, client_fd, &event);

	std::cout << "New connection: " << client_fd << std::endl;
}

void Multiplexer::queueResponse(int client_fd, const std::string& data) {
	_writeBuffers[client_fd] += data;
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLOUT;
	event.data.fd = client_fd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, client_fd, &event) == -1) {
		perror("epoll_ctl: queueResponse");
	}
}

bool Multiplexer::_hasCompletedRequest(int fd) const {
	const std::string& buffer = _readBuffers.at(fd);
	size_t header_end = buffer.find("\r\n\r\n");
	if (header_end == std::string::npos) return false;

	size_t content_length = 0;
	size_t cl_pos = buffer.find("Content-Length:");
	if (cl_pos != std::string::npos) {
		size_t line_end = buffer.find("\r\n", cl_pos);
		if (line_end != std::string::npos) {
			std::string cl_line = buffer.substr(cl_pos, line_end - cl_pos);
			content_length = std::atoi(cl_line.substr(15).c_str());
		}
	}

	size_t total_expected = header_end + 4 + content_length;
	return buffer.size() >= total_expected;
}

std::string Multiplexer::_extractCompleteRequest(int fd) {
	std::string& buffer = _readBuffers[fd];
	size_t header_end = buffer.find("\r\n\r\n");
	if (header_end == std::string::npos) return "";

	size_t content_length = 0;
	size_t cl_pos = buffer.find("Content-Length:");
	if (cl_pos != std::string::npos) {
		size_t line_end = buffer.find("\r\n", cl_pos);
		if (line_end != std::string::npos) {
			std::string cl_line = buffer.substr(cl_pos, line_end - cl_pos);
			content_length = std::atoi(cl_line.substr(15).c_str());
		}
	}

	size_t total_length = header_end + 4 + content_length;
	if (buffer.size() < total_length) return "";

	std::string request = buffer.substr(0, total_length);
	buffer.erase(0, total_length);
	return request;
}

void Multiplexer::_handleClientRead(int client_fd) {
	char buf[4096];
	ssize_t n;

	size_t maxBodySize = _getMaxBodySizeForClient(client_fd);

	while ((n = recv(client_fd, buf, sizeof(buf), 0)) > 0) {
		if (_readBuffers[client_fd].size() + n > maxBodySize && maxBodySize > 0) {
			_webServer->_sendError(client_fd, 413, "Request Entity Too Large");
			_closeFd(client_fd);
			return;
		}
		_readBuffers[client_fd].append(buf, n);
	}

	if (n == 0) {
		_closeFd(client_fd);
		return;
	} else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
		_closeFd(client_fd);
		return;
	}

	while (_hasCompletedRequest(client_fd)) {
		std::string request = _extractCompleteRequest(client_fd);
		if (!request.empty()) _webServer->processRequest(client_fd, request);
	}
}

void Multiplexer::_handleClientWrite(int client_fd) {
	std::string &out = _writeBuffers[client_fd];
	Request &req = _requests[client_fd];

	while (!out.empty()) {
		ssize_t sent = send(client_fd, out.data(), out.size(), 0);
		if (sent > 0) {
			out.erase(0, sent);
		} else if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			return;
		} else {
			_closeFd(client_fd);
			return;
		}
	}

	bool keepAlive = false;
	std::string connHeader = req.getHeader("Connection");
	if (!connHeader.empty() && (connHeader == "keep-alive" || connHeader == "Keep-Alive")) {
		keepAlive = true;
	}

	if (keepAlive) {
		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = client_fd;
		epoll_ctl(_epollFd, EPOLL_CTL_MOD, client_fd, &ev);

		req.reset();
		_writeBuffers[client_fd].clear();
	} else {
		_closeFd(client_fd);
	}
}

void Multiplexer::_closeFd(int client_fd) {
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, client_fd, NULL);
	::close(client_fd);
	if (_clientSockets.count(client_fd)) {
		delete _clientSockets[client_fd];
		_clientSockets.erase(client_fd);
	}
	_readBuffers.erase(client_fd);
	_writeBuffers.erase(client_fd);
	_clientToServerFd.erase(client_fd);
}

size_t Multiplexer::_getMaxBodySizeForClient(int client_fd) const {
	return _webServer->_getMaxBodySizeForClient(client_fd);
}

std::queue< std::pair<pid_t, int> > Multiplexer::_sigchldQueue;

void Multiplexer::_sigchldHandler(int signo) {
	(void)signo; // unused

	int status;
	pid_t pid;

	// Collect all dead children
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		_sigchldQueue.push(std::make_pair(pid, status));
	}
}

void Multiplexer::processSigchldQueue() {
	while (!_sigchldQueue.empty()) {
		std::pair<pid_t, int> info = _sigchldQueue.front();
		_sigchldQueue.pop();
		reapChild(info.first, info.second);
	}
}

void Multiplexer::reapChild(pid_t pid, int status) {
	if (WIFEXITED(status)) {
		std::cout << "Child " << pid
				  << " exited with code " << WEXITSTATUS(status) << std::endl;
	} else if (WIFSIGNALED(status)) {
		std::cout << "Child " << pid
				  << " killed by signal " << WTERMSIG(status) << std::endl;
	}
}

int Multiplexer::getServerFdForClient(int client_fd) const {
	std::map<int, int>::const_iterator it = _clientToServerFd.find(client_fd);

	if (it != _clientToServerFd.end())
		return it->second;
	return -1;
}