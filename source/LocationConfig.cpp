#include "../include/Parser.hpp"
#include "../include/ServerConfig.hpp"
#include "../include/LocationConfig.hpp"

LocationConfig::LocationConfig() :
	_path(""),
	_root(""),
	_index(""),
	_autoindex(),
	_uploadStore(""),
	_redirectCode(""),
	_redirectUrl(""),
	_clientMaxBodySize(0),
	_cgiExtensions(),
	_cgiInterpreters() {
}

LocationConfig::LocationConfig(const LocationConfig& source) :
	_path(source._path),
	_root(source._root),
	_index(source._index),
	_autoindex(source._autoindex),
	_allowedMethods(source._allowedMethods),
	_uploadStore(source._uploadStore),
	_redirectCode(source._redirectCode),
	_redirectUrl(source._redirectUrl),
	_clientMaxBodySize(source._clientMaxBodySize),
	_cgiExtensions(source._cgiExtensions),
	_cgiInterpreters(source._cgiInterpreters)
{}

LocationConfig& LocationConfig::operator=(const LocationConfig& rhs) {
	if (this != &rhs) {
		_path = rhs._path;
		_root = rhs._root;
		_index = rhs._index;
		_autoindex = rhs._autoindex;
		_allowedMethods = rhs._allowedMethods;
		_uploadStore = rhs._uploadStore;
		_redirectCode = rhs._redirectCode;
		_redirectUrl = rhs._redirectUrl;
		_clientMaxBodySize = rhs._clientMaxBodySize;
		_cgiExtensions = rhs._cgiExtensions;
		_cgiInterpreters = rhs._cgiInterpreters;
	}
	return *this;
}

LocationConfig::~LocationConfig() {}

bool LocationConfig::isCgiEnabled() const {
	return !_cgiExtensions.empty() && !_cgiInterpreters.empty();
}

const std::string& LocationConfig::getPath() const {
	return _path;
}

const std::string& LocationConfig::getRoot() const {
	return _root;
}

const std::string& LocationConfig::getIndex() const {
	return _index;
}

bool LocationConfig::getAutoindex() const {
	return _autoindex;
}

const std::set<std::string>& LocationConfig::getAllowedMethods() const {
	return _allowedMethods;
}

const std::string& LocationConfig::getUploadStore() const {
	return _uploadStore;
}

const std::string& LocationConfig::getRedirectCode() const {
	return _redirectCode;
}

const std::string& LocationConfig::getRedirectUrl() const {
	return _redirectUrl;
}

size_t LocationConfig::getClientMaxBodySize() const {
	return _clientMaxBodySize;
}

const std::vector<std::string>& LocationConfig::getCgiExtensions() const {
	return _cgiExtensions;
}

const std::vector<std::string>& LocationConfig::getCgiInterpreters() const {
	return _cgiInterpreters;
}

// use first cgi interpreter as executable path
// if interpreter is already an absolute path, return it directly
// if not, combine with root directory
std::string LocationConfig::getCgiPath() const {
	if (_cgiInterpreters.empty() || _root.empty()) {
		return "";
	}

	std::string interpreter = _cgiInterpreters[0];

	if (interpreter[0] == '/') {
		return interpreter;
	}

	std::string cgiPath = _root;
	if (cgiPath[cgiPath.length() - 1] != '/') {
		cgiPath += '/';
	}
	cgiPath += interpreter;

	return cgiPath;
}
