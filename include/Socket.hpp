#ifndef SOCKET_HPP
# define SOCKET_HPP

# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <fcntl.h>
# include <string>
# include <iostream>
# include <cerrno>

class Socket {
	private :
		int		_fd;

		Socket(const Socket &source);
		Socket& operator=(const Socket &rhs);

	public :
		Socket();
		Socket(int fd);
		~Socket();

		// socket operations
		bool	createSocket();
		bool	bindSocket(const std::string& host, int port);
		bool	listenSocket(int backlog);
		int		acceptConnection(struct sockaddr_in* client_addr, socklen_t* addr_len);
		void	closeSocket();
		bool	setNonBlocking();

		// getters
		int		getFd() const;
		bool	isValid() const;
};

#endif
