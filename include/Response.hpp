#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <string>
# include <sstream>
# include <iostream>
# include <map>
# include <algorithm>

class Response {
	private:
		std::string							_version;
		int									_statusCode;
		std::string							_statusMessage;
		std::map<std::string, std::string>	_headers;
		std::string							_body;

		Response(const Response &source);
		Response& operator=(const Response &rhs);

	public :
		Response();
		~Response();

		void	setStatus(int code, const std::string& message);
		void	setHeader(const std::string& key, const std::string& value);
		void	setBody(const std::string& body);
		void	setContentType(const std::string& type);


		int											getStatusCode() const;
		const std::string&							getStatusMessage() const;
		const std::string&							getVersion() const;
		const std::map<std::string, std::string>&	getHeaders() const;
		const std::string&							getBody() const;
		const std::string							_getMimeType(const std::string& filePath) const;

		std::string	toString() const;
};

#endif
