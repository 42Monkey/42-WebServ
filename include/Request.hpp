#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <string>
# include <map>
# include <vector>
# include <algorithm>
# include <cstring>
# include <cstdio>

class Request {
	private:
		std::string							_method;
		std::string							_path;
		std::string 						_uri;
		std::string							_version;
		std::map<std::string, std::string>	_headers;
		std::string							_body;
		std::string							_query;
		bool								_isComplete;
		int 								_clientFd;

		void	_parseRequestLine(const std::string& line);

		std::string	_toLower(const std::string& string) const;

	public :
		Request();
		Request(int clientFd, const std::string &raw);
		Request(const Request &source);
		Request& operator=(const Request &rhs);
		~Request();

		bool parse(const std::string &raw);
		void reset();

		void setClientFd(int fd);

		// Getters
		int 										getClientFd() const;
		const std::string&							getMethod() const;
		const std::string&							getPath() const;
		const std::string&							getVersion() const;
		const std::string&							getUri() const;
		const std::string&							getQuery() const;
		std::string									getHeader(const std::string &key) const;
		const std::map<std::string, std::string>&	getHeaders() const;
		const std::string&							getBody() const;

		bool isComplete() const;
};

#endif
