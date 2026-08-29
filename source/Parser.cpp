#include "../include/Parser.hpp"
#include "../include/ServerConfig.hpp"
#include "../include/LocationConfig.hpp"
#include "../include/Constants.hpp"
#include <sstream>
#include <stdexcept>

// constructor
Parser::Parser(const std::vector<Lexer::Token>& tokens) : _tokens(tokens), _currentTokenIndex(0) {}

// main parsing method
std::vector<ServerConfig> Parser::parse() {
	return parseConfigFile();
}

// navigation methods
const Lexer::Token& Parser::currentToken() const {
	if (isEOF()) {
		throwParseError("Unexpected end of token stream (EOF).");
	}
	return _tokens[_currentTokenIndex];
}

void Parser::advance() {
	if (!isEOF()) {
		_currentTokenIndex++;
	}
}

bool Parser::isEOF() const {
	return _currentTokenIndex >= _tokens.size() || _tokens[_currentTokenIndex]._type == Lexer::TOKEN_EOF;
}

// Handles error
void Parser::throwParseError(const std::string &message) const {
	std::stringstream ss;

	if (!isEOF()) {
		const Lexer::Token& token = currentToken();
		ss << "Parsing error at line " << token._line << ", column " << token._column << ": " << message << " (Found: " << Lexer::tokenTypeToString(token._type) << " '" << token._value << "')";
	} else {
		ss << "Parsing error: " << message;
	}
	throw std::runtime_error(ss.str());
}

// Consume methods
void Parser::consume(Lexer::TokenType expectedType, const std::string &message) {
	if (isEOF()) {
		throwParseError("Unexpected EOF while expecting " + message);
	}
	if (currentToken()._type != expectedType) {
		throwParseError(message);
	}
	advance();
}

void Parser::skipComments() {
	while (!isEOF() && currentToken()._type == Lexer::TOKEN_COMMENT) {
		advance();
	}
}

// Parsing Blocks
std::vector<ServerConfig> Parser::parseConfigFile() {
	std::vector<ServerConfig> server_config;

	while (!isEOF()) {
		skipComments();
		if (isEOF()) {
			break ;
		}
		if (currentToken()._type == Lexer::TOKEN_SERVER) {
			server_config.push_back(parseServerBlock());
		} else {
			throwParseError("Expected 'server' block or EOF");
		}
	}
	if (server_config.empty()) {
		throwParseError("Config file must contain at least one server block.");
	}
	checkDuplicateServerNames(server_config);
	return server_config;
}

ServerConfig Parser::parseServerBlock() {
	ServerConfig server;
	consume(Lexer::TOKEN_SERVER, "Expected 'server' keyword.");
	consume(Lexer::TOKEN_LBRACE, "Expected '{' after 'server' keyword.");

	bool listening = false;

	while (!isEOF() && currentToken()._type != Lexer::TOKEN_RBRACE) {
		skipComments();
		if (isEOF())
			throwParseError("Unexpected EOF in server block");

		if (currentToken()._type == Lexer::TOKEN_LOCATION) {
			server._locations.push_back(parseLocationBlock());
		} else {
			if (currentToken()._type == Lexer::TOKEN_LISTEN) {
				listening = true;
				parseListenDirective(server);
			} else {
				parseServerDirective(server);
			}
		}
	}
	setServerDefault(server);
	if (!listening) {
		throwParseError("Server block must contain at least one listen directive");
	}
	checkDuplicatePorts(server);
	checkDuplicatePaths(server._locations);
	consume(Lexer::TOKEN_RBRACE, "Expected '}' to close server block.");
	return server;
}

// path validation
// parse directives
// validate before setting defaults
LocationConfig Parser::parseLocationBlock() {
	LocationConfig location;
	consume(Lexer::TOKEN_LOCATION, "Expected 'location' keyword.");

	if (currentToken()._type != Lexer::TOKEN_PATH && currentToken()._type != Lexer::TOKEN_STRING) {
		throwParseError("Expected location path");
	}

	if (currentToken()._value.empty() || currentToken()._value[0] != '/') {
		throwParseError("Location path must start with a '/'");
	}

	location._path = currentToken()._value;
	advance();

	consume(Lexer::TOKEN_LBRACE, "Expected '{' after location path.");
	while (!isEOF() && currentToken()._type != Lexer::TOKEN_RBRACE) {
		skipComments();
		parseLocationDirective(location);
	}
	consume(Lexer::TOKEN_RBRACE, "Expected '}' to close location block.");

	if (!location._cgiExtensions.empty()) {
		checkCgi(location);
	}

	setLocationDefault(location);
	return location;
}

// Parsing Directives
void Parser::parseServerDirective(ServerConfig &server) {
	if (currentToken()._type == Lexer::TOKEN_LISTEN) {
		parseListenDirective(server);
	} else if (currentToken()._type == Lexer::TOKEN_SERVER_NAME) {
		parseServerNameDirective(server);
	} else if (currentToken()._type == Lexer::TOKEN_ERROR_PAGE) {
		parseErrorPageDirective(server);
	} else if (currentToken()._type == Lexer::TOKEN_CLIENT_MAX) {
		parseClientMaxBodySizeDirective(server);
	} else if (currentToken()._type == Lexer::TOKEN_HOST) {
		parseHostDirective(server);
	} else {
		throwParseError("Unknown or unexpected directive '" + currentToken()._value + "' within server block.");
	}
}

void Parser::parseListenDirective(ServerConfig &server) {
	consume(Lexer::TOKEN_LISTEN, "Expected 'listen' keyword");

	std::string value = currentToken()._value;
	size_t colonPosition = value.find(':');

	if (colonPosition == std::string::npos) {
		server._listenPorts.push_back(parsePort(value));
	} else {
		server._host = value.substr(0, colonPosition);
		server._listenPorts.push_back(parsePort(value.substr(colonPosition + 1)));
	}
	advance();
	consume(Lexer::TOKEN_SEMICOLON, "Expected ';' after listen directive.");
}

// server_name temu.com;
void Parser::parseServerNameDirective(ServerConfig &server) {
	consume(Lexer::TOKEN_SERVER_NAME, "Expected 'server_name' keyword.");

	while (currentToken()._type == Lexer::TOKEN_STRING || currentToken()._type == Lexer::TOKEN_NUMBER) {
		server._serverNames.push_back(currentToken()._value);
		advance();
	}
	if (server._serverNames.empty()) {
		throwParseError("At least one server name required.");
	}
	consume(Lexer::TOKEN_SEMICOLON, "Expected ';' after server_name directive.");
}

// error_page 400 /errors/400.html;
void Parser::parseErrorPageDirective(ServerConfig& server) {
	consume(Lexer::TOKEN_ERROR_PAGE, "Expected 'error_page' keyword.");

	int status = parseStatusCode(currentToken()._value);
	advance();

	if (currentToken()._type != Lexer::TOKEN_PATH) {
		throwParseError("Expected error page path");
	}
	server._errorPages[status] = currentToken()._value;
	advance();
	consume(Lexer::TOKEN_SEMICOLON, "Expected ';' after error_page directive.");
}

void Parser::parseClientMaxBodySizeDirective(ServerConfig &server) {
	parseClientMaxBodySize(server._clientMaxBodySize, 1073741824);
}

void Parser::parseLocationDirective(LocationConfig &location) {
	Lexer::TokenType type = currentToken()._type;

	if (type == Lexer::TOKEN_ROOT) {
		parseRootDirective(location);
	} else if (type == Lexer::TOKEN_INDEX) {
		parseIndexDirective(location);
	} else if (type == Lexer::TOKEN_AUTOINDEX) {
		parseAutoindexDirective(location);
	} else if (type == Lexer::TOKEN_METHODS) {
		parseMethodsDirective(location);
	} else if (type == Lexer::TOKEN_UPLOAD_STORE) {
		parseUploadStoreDirective(location);
	} else if (type == Lexer::TOKEN_RETURN) {
		parseReturnDirective(location);
	} else if (type == Lexer::TOKEN_CGI_PASS) {
		parseCgiPassDirective(location);
	} else if (type == Lexer::TOKEN_CGI_EXTENSION) {
		parseCgiExtensionDirective(location);
	} else if (type == Lexer::TOKEN_CLIENT_MAX) {
		parseClientMaxBodySizeDirective(location);
	} else {
		throwParseError("Unknown or unexpected directive '" + currentToken()._value + "' within location block.");
	}
}

// handle location directive
// root /var/www/html;
void Parser::parseRootDirective(LocationConfig &location) {
	consume(Lexer::TOKEN_ROOT, "Expected 'root' keyword.");

	if (currentToken()._type == Lexer::TOKEN_STRING || currentToken()._type == Lexer::TOKEN_PATH) {
		std::string path = currentToken()._value;
		advance();

		checkPath(path);
		location._root = path;
	} else {
		throwParseError("Expected path after 'root'");
	}
	consume(Lexer::TOKEN_SEMICOLON, "Expected ';' after root directive.");
}

// index index.html;
void Parser::parseIndexDirective(LocationConfig &location) {
	consume(Lexer::TOKEN_INDEX, "Expected 'index' keyword.");

	if (currentToken()._type == Lexer::TOKEN_STRING || currentToken()._type == Lexer::TOKEN_PATH) {
		location._index = currentToken()._value;
		advance();
	} else {
		throwParseError("Expected path after 'index'");
	}
	consume(Lexer::TOKEN_SEMICOLON, "Expected ';' after index directive.");
}

// methods GET POST DELETE;
void Parser::parseMethodsDirective(LocationConfig &location) {
	consume(Lexer::TOKEN_METHODS, "Expected 'methods' keyword");

	bool isMethod = false;
	while (!isEOF() && currentToken()._type == Lexer::TOKEN_STRING) {
		std::string method = currentToken()._value;
		if (method == "GET" || method == "POST" || method == "DELETE") {
			location._allowedMethods.insert(method);
			isMethod = true;
		} else {
			throwParseError("Invalid HTTP method.");
		}
		advance();
	}
	if (!isMethod) {
		throwParseError("Expected a valid method 'GET', 'POST', 'DELETE'");
	}
	consume(Lexer::TOKEN_SEMICOLON, "Expected ';' after methods directive");
}

// autoindex on;
void Parser::parseAutoindexDirective(LocationConfig &location) {
	consume(Lexer::TOKEN_AUTOINDEX, "Expected 'autoindex' keyword.");

	if (currentToken()._type == Lexer::TOKEN_STRING) {
		location._autoindex = parseBool(currentToken()._value);
		advance();
	} else {
		throwParseError("Expected on/off after 'autoindex'");
	}
	consume(Lexer::TOKEN_SEMICOLON, "Expected ';' after autoindex directive.");
}

void Parser::parseUploadStoreDirective(LocationConfig &location) {
	consume(Lexer::TOKEN_UPLOAD_STORE, "Expected 'upload_store' keyword.");

	if (currentToken()._type == Lexer::TOKEN_STRING || currentToken()._type == Lexer::TOKEN_PATH) {
		std::string path = currentToken()._value;
		advance();
		checkPath(path);
		location._uploadStore = path;
	} else {
		throwParseError("Expected path after 'upload_store'");
	}
	consume(Lexer::TOKEN_SEMICOLON, "Expected ';' after upload_store directive.");
}

// return 301 /new-location;
void Parser::parseReturnDirective(LocationConfig &location) {
	consume(Lexer::TOKEN_RETURN, "Expected 'return' keyword");

	if (currentToken()._type == Lexer::TOKEN_NUMBER) {
		int status = parseStatusCode(currentToken()._value);

		std::ostringstream oss;
		oss << status;
		location._redirectCode = oss.str();
		advance();
	} else {
		throwParseError("Expected numeric status code after 'return'");
	}

	if (currentToken()._type == Lexer::TOKEN_STRING || currentToken()._type == Lexer::TOKEN_PATH) {
		location._redirectUrl = currentToken()._value;
		advance();
	} else {
		throwParseError("Expected URL after status code in 'return directive.");
	}
	consume(Lexer::TOKEN_SEMICOLON, "Expected ';' after return directive.");
}

void Parser::parseCgiPassDirective(LocationConfig &location) {
	consume(Lexer::TOKEN_CGI_PASS, "Expected 'cgi_pass' keyword");

	if (currentToken()._type != Lexer::TOKEN_STRING && currentToken()._type != Lexer::TOKEN_PATH) {
		throwParseError("Expected path to CGI interpreter");
	}

	location._cgiInterpreters.push_back(currentToken()._value);
	advance();
	consume(Lexer::TOKEN_SEMICOLON, "Expected ';' after cgi_pass directive");
}

void Parser::parseCgiExtensionDirective(LocationConfig &location) {
	consume(Lexer::TOKEN_CGI_EXTENSION, "Expected 'cgi_extension' keyword");

	if (location._cgiInterpreters.empty()) {
		throwParseError("cgi_extension must come after cgi_pass");
	}

	if (currentToken()._type != Lexer::TOKEN_STRING) {
		throwParseError("Expected CGI extension");
	}

	if (currentToken()._value.empty() || currentToken()._value[0] != '.') {
		throwParseError("Invalid CGI extension");
	}
	location._cgiExtensions.push_back(currentToken()._value);
	advance();
	consume(Lexer::TOKEN_SEMICOLON, "Expected ';', after cgi_extension directive");
}

void Parser::parseClientMaxBodySize(size_t &configSize, size_t maxSize) {
	consume(Lexer::TOKEN_CLIENT_MAX, "Expected 'client_max_body_size' keyword");

	char *end;
	size_t size = strtol(currentToken()._value.c_str(), &end, 10);

	if (end == currentToken()._value.c_str() || *end != '\0' || size <= 0) {
		throwParseError("invalid client max body size value");
	}

	advance();
	if (!isEOF() && currentToken()._type == Lexer::TOKEN_STRING) {
		std::string unit = currentToken()._value;
		if (unit == "k" || unit == "K") {
			size *= 1024;
		} else if (unit == "m" || unit == "M") {
			size *= 1024 * 1024;
		} else {
			throwParseError("Invalid unit (use k/K/m/M)");
		}
		advance();
	}
	if (size > maxSize) {
		throwParseError("Upload size exceeds limit (10mb)");
	}
	configSize = size;
	consume(Lexer::TOKEN_SEMICOLON, "Expected ';' after client_max_body_size directive.");
}

void Parser::parseClientMaxBodySizeDirective(LocationConfig &location) {
	parseClientMaxBodySize(location._clientMaxBodySize, MAX_UPLOAD_SIZE);
}


void Parser::parseHostDirective(ServerConfig &server) {
	consume(Lexer::TOKEN_HOST, "Expected host keyword");

	if (currentToken()._type != Lexer::TOKEN_IP) {
		throwParseError("Expected an IP address after 'host' directive");
	}

	server._host = currentToken()._value;
	advance();
	consume(Lexer::TOKEN_SEMICOLON, "Expected ';' after host directive");
}

// set defaults
// do not set default for listen, mandatory in the subject
// do not set default for host, no required in the subject
void Parser::setServerDefault(ServerConfig& server) const {
	if (server._errorPages.empty()) {
		server._errorPages[404] = "/error_pages/404.html";
	}
}

void Parser::setLocationDefault(LocationConfig& location) const {
	if (location._allowedMethods.empty()) {
		location._allowedMethods.insert("GET");
		location._allowedMethods.insert("POST");
		location._allowedMethods.insert("DELETE");
	}

	if (location._redirectCode.empty() && location._index.empty()) {
		location._index = "index.html";
	}
}

// Parser utils
int Parser::parsePort(const std::string &string) const {
	int port = std::atoi(string.c_str());

	if (port < 1 || port > 65535) {
		throwParseError("Port number is out of range (1-65535): " + string);
	}
	return port;
}

int Parser::parseStatusCode(const std::string &string) const {
	int status = std::atoi(string.c_str());

	if (status < 100 || status > 599) {
		throwParseError("Invalid HTTP status code : " + string);
	}
	return status;
}

bool Parser::parseBool(const std::string &value) const {
	if (value == "on")
		return true;
	if (value == "off")
		return false;
	throwParseError("Expected 'on' or 'off' for boolean: " + value);
	return false;
}

void Parser::checkPath(std::string &path) const {
	while (!path.empty() && path[path.length() - 1] == '/') {
		path.resize(path.length() - 1);
	}

	if (path.find("..") != std::string::npos) {
		throwParseError("Path traversal ('..') not allowed");
	}

	if (path.empty()) {
		throwParseError("Path cannot be empty");
	}

	if (path[0] == '/') {
		return;
	}

	if (path.length() >= 2 && path.substr(0, 2) == "./") {
		char* cwd = getcwd(NULL, 0);
		if (cwd == NULL) {
			throwParseError("Failed to get current working directory");
		}

		std::string absolutePath = std::string(cwd) + "/" + path.substr(2);
		free(cwd);
		path = absolutePath;
		return;
	}

	if (path[0] != '/') {
		char* cwd = getcwd(NULL, 0);
		if (cwd == NULL) {
			throwParseError("Failed to get current working directory");
		}

		std::string absolutePath = std::string(cwd) + "/" + path;
		free(cwd);
		path = absolutePath;
		return;
	}
}

void Parser::checkBraces() const {
	int count = 0;
	for (size_t i = 0; i < _tokens.size(); ++i) {
		if (_tokens[i]._type == Lexer::TOKEN_LBRACE) {
			count++;
		}
		if (_tokens[i]._type == Lexer::TOKEN_RBRACE) {
			count--;
		}
	}
	if (count != 0) {
		throwParseError("Unmatched braces count");
	}
}

void Parser::checkDuplicatePorts(const ServerConfig &server) const {
	for (size_t i = 0; i < server._listenPorts.size(); ++i) {
		for (size_t j = i + 1; j < server._listenPorts.size(); ++j) {
			if (server._listenPorts[i] == server._listenPorts[j]) {
				throwParseError("Duplicate listen port in server block");
			}
		}
	}
}

void Parser::checkDuplicatePaths(const std::vector<LocationConfig> &locations) const {
	for (size_t i = 0; i < locations.size(); ++i) {
		for (size_t j = i + 1; j < locations.size(); ++j) {
			if (locations[i]._path == locations[j]._path) {
				throwParseError("Duplicate location paths in server block");
			}
		}
	}
}

void Parser::checkDuplicateServerNames(const std::vector<ServerConfig> &servers) const {
	for (size_t i = 0; i < servers.size(); ++i) {
		for (size_t j = i + 1; j < servers.size(); ++j) {
			for (size_t k = 0; k < servers[i]._serverNames.size(); ++k) {
				for (size_t l = 0; l < servers[j]._serverNames.size(); ++l) {
					if (servers[i]._serverNames[k] == servers[j]._serverNames[l])
						throwParseError("Duplicate server_name found");
				}
			}
		}
	}
}

void Parser::checkCgi(const LocationConfig &location) const {
	if (location._cgiExtensions.size() > location._cgiInterpreters.size()) {
		throwParseError("Missing cgi extension");
	}

	for (size_t i = 0; i < location._cgiExtensions.size(); ++i) {
		for (size_t j = i + 1; j < location._cgiExtensions.size(); ++j) {
			if (location._cgiExtensions[i] == location._cgiExtensions[j]) {
				throwParseError("Duplicate CGI found");
			}
		}
	}
}
