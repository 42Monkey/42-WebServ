#include "Socket.hpp"
#include <cstring>
#include <netinet/in.h>

Socket::Socket() : _fd(-1) {}

Socket::Socket(int fd) : _fd(fd) {}

Socket::~Socket() {
	closeSocket();
}

bool Socket::createSocket() {
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd == -1) {
		std::cerr << "socket() failed" << std::endl;
		return false;
	}

	int opt = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		std::cerr << "setsockopt(SO_REUSEADDR) failed" << std::endl;
		closeSocket();
		return false;
	}
	return true;
}

bool Socket::bindSocket(const std::string& host, int port) {
	struct sockaddr_in address;

	if (_fd == -1) {
		std::cerr << "Socket not created" << std::endl;
		return false;
	}

	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);

	if (host.empty() || host == "localhost") {
		address.sin_addr.s_addr = INADDR_ANY;
	} else {
		address.sin_addr.s_addr = inet_addr(host.c_str());
		if (address.sin_addr.s_addr == INADDR_NONE) {
			std::cerr << "Invalid host address: " << host << std::endl;
			return false;
		}
	}
    if (bind(_fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
		std::cerr << "bind() failed." << std::endl;
		return false;
	}
	return true;

}

bool Socket::listenSocket(int backlog) {
	if (_fd == -1) {
		std::cerr << "Socket not created" << std::endl;
		return false;
	}

	if (listen(_fd, backlog) == -1) {
		std::cerr << "listen() failed." << std::endl;
		return false;
	}
	return true;
}

int Socket::acceptConnection(struct sockaddr_in* client_addr, socklen_t* addr_len) {
	int client_fd;

	if (_fd == -1) {
		std::cerr << "Socket not created" << std::endl;
		return -1;
	}

	client_fd = accept(_fd, (struct sockaddr*)client_addr, addr_len);
	if (client_fd == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			std::cerr  << "accept() failed : " << strerror(errno) << std::endl;
		}
		return -1;
	}
	return client_fd;
}

void Socket::closeSocket() {
	if (_fd != -1) {
		close(_fd);
		_fd = -1;
	}
}

bool Socket::setNonBlocking() {
	int flags;

	if (_fd == -1) {
		std::cerr << "Socket not created" << std::endl;
		return false;
	}

	flags = fcntl(_fd, F_GETFL, 0);
	if (flags == -1) {
		std::cerr << "Error : failed to get file descriptor flags : " << strerror(errno) << std::endl;
		return false;
	}

	if (fcntl(_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		std::cerr << "Error : failed to set non-blocking mode : " << strerror(errno) << std::endl;
		return false;
	}
	return true;
}

int Socket::getFd() const {
	return _fd;
}

bool Socket::isValid() const {
	return _fd != -1;
}
