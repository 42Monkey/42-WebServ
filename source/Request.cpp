#include "../include/Request.hpp"
#include <sstream>

Request::Request() : _isComplete(false), _clientFd(-1) {}

Request::Request(int clientFd, const std::string &raw) : _isComplete(false), _clientFd(clientFd) {
	parse(raw);
}

Request::Request(const Request &source) {
	*this = source;
}

Request& Request::operator=(const Request &rhs) {
	if (this != &rhs) {
		_method = rhs._method;
		_path = rhs._path;
		_version = rhs._version;
		_uri = rhs._uri;
		_query = rhs._query;
		_headers = rhs._headers;
		_body = rhs._body;
		_isComplete = rhs._isComplete;
		_clientFd = rhs._clientFd;
	}
	return *this;
}

Request::~Request() {}

bool Request::parse(const std::string &raw) {
	size_t headerEnd = raw.find("\r\n\r\n");
	if (headerEnd == std::string::npos) {
		_isComplete = false;
		return false;
	}

	std::string headerPart = raw.substr(0, headerEnd);
	_body = raw.substr(headerEnd + 4);

	std::istringstream stream(headerPart);
	std::string line;

	if (!std::getline(stream, line)) {
		_isComplete = false;
		return false;
	}
	_parseRequestLine(line);

	while (std::getline(stream, line) && !line.empty()) {
		if (!line.empty() && line[line.size() - 1] == '\r') {
			line.resize(line.size() - 1);
		}
		size_t colon = line.find(':');
		if (colon != std::string::npos) {
			std::string key = line.substr(0, colon);
			std::string value = line.substr(colon + 1);
			size_t start = value.find_first_not_of(" \t");
			if (start != std::string::npos) {
				value = value.substr(start);
			}
			_headers[_toLower(key)] = value;
		}
	}
	_isComplete = true;
	return true;
}

void Request::_parseRequestLine(const std::string &line) {
	std::istringstream ss(line);
	ss >> _method >> _uri >> _version;

	size_t queryPosition = _uri.find('?');
	if (queryPosition != std::string::npos) {
		_query = _uri.substr(queryPosition + 1);
		_path = _uri.substr(0, queryPosition);
	} else {
		_path = _uri;
	}
	std::transform(_method.begin(), _method.end(), _method.begin(), ::toupper);
}

std::string Request::_toLower(const std::string &str) const {
	std::string lower = str;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
	return lower;
}

void Request::reset() {
	_method.clear();
	_path.clear();
	_version.clear();
	_uri.clear();
	_query.clear();
	_headers.clear();
	_body.clear();
	_isComplete = false;
}

// Getters
int Request::getClientFd() const {
	return _clientFd;
}

const std::string& Request::getMethod() const {
	return _method;
}

const std::string& Request::getPath() const {
	return _path;
}

const std::string& Request::getVersion() const {
	return _version;
}

const std::string& Request::getUri() const {
	return _uri;
}

const std::string& Request::getQuery() const {
	return _query;
}

std::string Request::getHeader(const std::string &key) const {
	std::string lowerKey = _toLower(key);
	std::map<std::string, std::string>::const_iterator it = _headers.find(lowerKey);
	if (it != _headers.end()) {
		return it->second;
	}
	return "";
}

const std::map<std::string, std::string>& Request::getHeaders() const {
	return _headers;
}

const std::string& Request::getBody() const {
	return _body;
}

bool Request::isComplete() const {
	return _isComplete;
}

void Request::setClientFd(int fd) {
	_clientFd = fd;
}
