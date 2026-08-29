#ifndef MULTIPLEXER_HPP
# define MULTIPLEXER_HPP

# include <vector>
# include <map>
# include <string>
# include <sys/wait.h>
# include <sys/epoll.h>
# include <unistd.h>
# include <sstream>
# include <signal.h>
# include <queue>
# include "../include/CGI.hpp"
# include "../include/Request.hpp"
# include "../include/LocationConfig.hpp"

class Request;
class Socket;
class WebServer;

enum FileOperationType {
    FILE_READ,
    FILE_WRITE
};

struct FileOperation {
    FileOperationType type;
    int client_fd;
    std::string file_path;
    std::string data;
    size_t bytes_processed;
    int file_fd;
    bool headers_sent;

    FileOperation() : type(FILE_READ), client_fd(-1), bytes_processed(0), file_fd(-1), headers_sent(false) {}
};

class Multiplexer {
    public:
        Multiplexer(WebServer* server);
        ~Multiplexer();

        bool    addServerSocket(int server_fd);
        void    addCgi(int client_fd, int pipe_fd, CGI* cgi);
        void    run();
        void    queueResponse(int client_fd, const std::string& data);

        // Async file operations
        bool    startFileRead(int client_fd, const std::string& file_path);
        bool    startFileWrite(int client_fd, const std::string& file_path, const std::string& data);

        void reapChild(pid_t pid, int status);
        void processSigchldQueue();
        int getServerFdForClient(int client_fd) const;

    private:
        WebServer*   _webServer;
        bool         _running;

        // socket collection
        std::vector<int>    _serverFds;

        // epoll fd
        int                 _epollFd;

        // CGI management
        std::map<int, std::pair<int, CGI*> > _cgiPipes;
        std::map<int, CGI*>                  _cgiPidMap;
        static std::queue< std::pair<pid_t, int> > _sigchldQueue;
        std::map<int, Request>               _requests;

        // File operations management
        std::map<int, FileOperation>    _fileOperations;

        // client data management
        std::map<int, size_t>       _clientToConfig;
        std::map<int, std::string>  _readBuffers;
        std::map<int, std::string>  _writeBuffers;
        std::map<int, bool>         _cgiHeadersSent;
        std::map<int, Socket*>      _clientSockets;
        std::map<int, int>          _clientToServerFd;

        // event handlers
        void    _handleNewConnection(int server_fd);
        void    _handleClientRead(int client_fd);
        void    _handleClientWrite(int client_fd);
        void    _handleCgiPipeRead(int pipe_fd);
        void    _handleFileRead(int file_fd);
        void    _handleFileWrite(int file_fd);
        void    _closeFd(int client_fd);
        void    _closeFileOperation(int file_fd);
        bool    _isServerSocket(int fd) const;
        bool    _isFileOperation(int fd) const;
        static void _sigchldHandler(int signo);

        // http request detection
        bool        _hasCompletedRequest(int fd) const;
        std::string _extractCompleteRequest(int fd);

        // file operation helpers
        std::string _getContentType(const std::string& file_path);
        bool        _sendFileHeaders(int client_fd, const std::string& file_path, size_t file_size);

        size_t _getMaxBodySizeForClient(int client_fd) const;

        Multiplexer(const Multiplexer &source);
        Multiplexer& operator=(const Multiplexer &rhs);
};

#endif
