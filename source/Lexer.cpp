#include "../include/Lexer.hpp"

Lexer::Lexer(const std::string &config)
	:	_input(config),
		_position(0),
		_line(1),
		_column(1),
		_hasError(false),
		_errorMessage("")
{}

// CORE METHOD
// reads strings and produces a vector of Tokens
std::vector<Lexer::Token> Lexer::tokenize() {
	std::vector<Token> tokens;

	while (_position < _input.length() && !_hasError) {
		skipWhiteSpace();

		if (_position >= _input.length()) {
			break ;
		}

		char c = currentChar();

		if (c == '{') {
			tokens.push_back(createToken(TOKEN_LBRACE, "{"));
			advance();
		} else if (c == '}') {
			tokens.push_back(createToken(TOKEN_RBRACE, "}"));
			advance();
		} else if (c == ';') {
			tokens.push_back(createToken(TOKEN_SEMICOLON, ";"));
			advance();
		} else if (c == '#') {
			skipComment();
		} else if (c == '"') {
			tokens.push_back(readString());
		} else if (isDigit(c)) {
			tokens.push_back(readNumber());
		} else if (c == '/') {
			tokens.push_back(readPath());
		}
		else if (isAlpha(c) || c == '.' || c == '-') {
			tokens.push_back(readWord());
		}
		else {
			tokens.push_back(createToken(TOKEN_UNKNOWN, std::string(1, c)));
			advance();
		}

		if (_hasError) {
			tokens.clear();
			break ;
		}
	}

	if (!_hasError) {
		tokens.push_back(createToken(TOKEN_EOF, ""));
	}

	return tokens;
}

// CREATE TOKENS
Lexer::Token Lexer::createToken(TokenType type, const std::string &value) const {
	return Token(type, value, _line, _column - value.length());
}

// Token reading methods
// determines the TokenType
// if not a keyword, defaults to TOKEN_STRING and let parser handle it
Lexer::TokenType Lexer::getType(const std::string &word) const {
	if (word == "server")
		return TOKEN_SERVER;
	if (word == "location")
		return TOKEN_LOCATION;
	if (word == "listen")
		return TOKEN_LISTEN;
	if (word == "server_name")
		return TOKEN_SERVER_NAME;
	if (word == "error_page")
		return TOKEN_ERROR_PAGE;
	if (word == "client_max_body_size")
		return TOKEN_CLIENT_MAX;
	if (word == "root")
		return TOKEN_ROOT;
	if (word == "index")
		return TOKEN_INDEX;
	if (word == "autoindex")
		return TOKEN_AUTOINDEX;
	if (word == "allowed_methods")
		return TOKEN_METHODS;
	if (word == "upload_store")
		return TOKEN_UPLOAD_STORE;
	if (word == "return")
		return TOKEN_RETURN;
	if (word == "cgi_pass")
		return TOKEN_CGI_PASS;
	if (word == "cgi_extension")
		return TOKEN_CGI_EXTENSION;
	if (word == "host")
		return TOKEN_HOST;

	return TOKEN_STRING;
}

// reads a quoted string
Lexer::Token Lexer::readString() {
	size_t		start = _column;
	std::string	value = "";

	advance();
	while (currentChar() != '"' && currentChar() != '\0') {
		if (currentChar() == '\n') {
			setError("New line dectected before closing quote.");
			return Token(TOKEN_UNKNOWN, "", _line, start);
		}
		value += currentChar();
		advance();
	}

	if (currentChar() == '\0') {
		setError("EOF reached before closing quote.");
		return Token(TOKEN_UNKNOWN, "", _line, start);
	}
	advance();
	return Token(TOKEN_STRING, value, _line, start);
}

Lexer::Token Lexer::readNumber() {
	size_t		start = _column;
	std::string	value = "";

	std::string tempIP = "";
	size_t		tempPosition = _position;

	while (tempPosition < _input.length() && (isDigit(_input[tempPosition]) || _input[tempPosition] == '.' || _input[tempPosition] == ':')) {
		tempIP += _input[tempPosition];
		tempPosition++;
	}

	if (isIP(tempIP)) {
		_position = tempPosition;
		_column += tempIP.length();
		return Token(TOKEN_IP, tempIP, _line, start);
	}

	while (isDigit(currentChar())) {
		value += currentChar();
		advance();
	}

	if (value.empty()) {
		setError("Empty, no digits found.");
		return Token(TOKEN_UNKNOWN, "", _line, start);
	}
	return createToken(TOKEN_NUMBER, value);
}

Lexer::Token Lexer::readWord() {
	size_t		start = _column;
	std::string	value = "";

	while (isAlphaNumeric(currentChar()) || currentChar() == '_' || currentChar() == '.' || currentChar() == '-') {
		value += currentChar();
		advance();
	}

	if (value.empty()) {
		setError("No word found.");
		return Token(TOKEN_UNKNOWN, "", _line, start);
	}

	TokenType type = getType(value);
	return createToken(type, value);
}

Lexer::Token Lexer::readPath() {
	size_t		start = _column;
	std::string	value = "";

	while (isPathChar(currentChar())) {
		value += currentChar();
		advance();
	}
	if (value.empty()) {
		setError("No valid path characters found.");
		return Token(TOKEN_UNKNOWN, "", _line, start);
	}
	return createToken(TOKEN_PATH, value);
}

// Handles stream manipulation
char Lexer::currentChar() const {
	if (_position < _input.length()) {
		return _input[_position];
	}
	return '\0';
}

char Lexer::peekChar() const {
	if (_position + 1 < _input.length()) {
		return _input[_position + 1];
	}
	return '\0';
}

void Lexer::advance() {
	if (_position < _input.length()) {
		_position++;
		_column++;
	}
}

void Lexer::newLine() {
	advance();
	_line++;
	_column = 1;
}

void Lexer::skipWhiteSpace() {
	while (_position < _input.length() && isSpace(currentChar())) {
		if (currentChar() == '\n') {
			newLine();
		} else {
			advance();
		}
	}
}

void Lexer::skipComment() {
	advance();
	while (currentChar() != '\n' && currentChar() != '\0') {
		advance();
	}
}

// Handles Characters
bool Lexer::isSpace(char c) const {
	return c == ' ' || c == '\t' || c == '\n';
}

bool Lexer::isDigit(char c) const {
	return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) const {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool Lexer::isAlphaNumeric(char c) const {
	return isAlpha(c) || isDigit(c);
}

bool Lexer::isPathChar(char c) const {
	return isAlphaNumeric(c) || c == '/' || c == '.' || c == '-' || c == '_';
}

bool Lexer::isIP(const std::string &string) const {
	int count = 0;
	int number = 0;
	int hasDigit = 0;

	for (std::string::const_iterator it = string.begin(); it != string.end(); ++it) {
		char c = *it;

		if (c == '.' || c == ':') {
			if (!hasDigit || number > 255) {
				return false;
			}

			count++;
			if (c ==  ':') {
				return count == 4;
			}

			number = 0;
			hasDigit = 0;
		} else if (isDigit(c)) {
			number = number * 10 + (c - '0');
			hasDigit = 1;
		} else {
			return false;
		}

	}
	if (!hasDigit || number > 255) {
		return false;
	}
	count++;

	return count == 4;
}

// Handles Errors
void Lexer::setError(const std::string& message) {
	if (!_hasError) {
		_hasError = true;
		std::stringstream ss;
		ss << "Error at line " << _line << ", column " << _column << ": " << message;
		_errorMessage = ss.str();
	}
}

 std::string Lexer::tokenTypeToString(TokenType type) {
	switch (type) {
		case TOKEN_SERVER: return "TOKEN_SERVER";
		case TOKEN_LOCATION: return "TOKEN_LOCATION";
		case TOKEN_LBRACE: return "TOKEN_LBRACE";
		case TOKEN_RBRACE: return "TOKEN_RBRACE";
		case TOKEN_SEMICOLON: return "TOKEN_SEMICOLON";
		case TOKEN_LISTEN: return "TOKEN_LISTEN";
		case TOKEN_SERVER_NAME: return "TOKEN_SERVER_NAME";
		case TOKEN_ERROR_PAGE: return "TOKEN_ERROR_PAGE";
		case TOKEN_CLIENT_MAX: return "TOKEN_CLIENT_MAX";
		case TOKEN_ROOT: return "TOKEN_ROOT";
		case TOKEN_INDEX: return "TOKEN_INDEX";
		case TOKEN_AUTOINDEX: return "TOKEN_AUTOINDEX";
		case TOKEN_METHODS: return "TOKEN_METHODS";
		case TOKEN_UPLOAD_STORE: return "TOKEN_UPLOAD_STORE";
		case TOKEN_RETURN: return "TOKEN_RETURN";
		case TOKEN_CGI_PASS: return "TOKEN_CGI_PASS";
		case TOKEN_CGI_EXTENSION: return "TOKEN_CGI_EXTENSION";
		case TOKEN_STRING: return "TOKEN_STRING";
		case TOKEN_NUMBER: return "TOKEN_NUMBER";
		case TOKEN_PATH: return "TOKEN_PATH";
		case TOKEN_COMMENT: return "TOKEN_COMMENT";
		case TOKEN_EOF: return "TOKEN_EOF";
		case TOKEN_IP: return "TOKEN_IP";
		case TOKEN_HOST: return "TOKEN_HOST";
		case TOKEN_UNKNOWN: return "TOKEN_UNKNOWN";
		default: return "UNKNOWN_TYPE"; // Should not be reached
	}
 }
