#ifndef Lexer_HPP
# define Lexer_HPP

# include <string>
# include <vector>
# include <iostream>
# include <sstream>
# include <cstddef>
# include <stdexcept>

class Lexer {
	public :

		enum TokenType {
			// structural
			TOKEN_SERVER,
			TOKEN_LOCATION,
			TOKEN_LBRACE,
			TOKEN_RBRACE,
			TOKEN_SEMICOLON,

			TOKEN_IP,
			TOKEN_HOST,

			// server directives
			TOKEN_LISTEN,
			TOKEN_SERVER_NAME,
			TOKEN_ERROR_PAGE,
			TOKEN_CLIENT_MAX,

			// location directives
			TOKEN_ROOT,
			TOKEN_INDEX,
			TOKEN_AUTOINDEX,
			TOKEN_METHODS,
			TOKEN_UPLOAD_STORE,
			TOKEN_RETURN,
			TOKEN_CGI_PASS,
			TOKEN_CGI_EXTENSION,

			// values
			TOKEN_STRING,
			TOKEN_NUMBER,
			TOKEN_PATH,

			// special
			TOKEN_COMMENT,
			TOKEN_EOF,
			TOKEN_UNKNOWN
		};

		struct Token {
			TokenType	_type;
			std::string	_value;
			size_t		_line;
			size_t		_column;

			Token(TokenType type, const std::string &value, size_t line, size_t column) : _type(type), _value(value), _line(line), _column(column) {};
		};

		Lexer(const std::string &config);

		std::vector<Token> tokenize();

		static std::string tokenTypeToString(TokenType type);;


	private :
		std::string _input;
		size_t _position;
		size_t _line;
		size_t _column;

		bool _hasError;
		std::string _errorMessage;

		Token createToken(TokenType type, const std::string &value) const;

		TokenType getType(const std::string& word) const;
		Token readString();
		Token readNumber();
		Token readWord();
		Token readPath();

		char currentChar() const;
		char peekChar() const;
		void advance();
		void newLine();
		void skipWhiteSpace();
		void skipComment();

		bool isSpace(char c) const;
		bool isDigit(char c) const;
		bool isAlpha(char c) const;
		bool isAlphaNumeric(char c) const;
		bool isPathChar(char c) const;
		bool isIP(const std::string &string) const;

		void setError(const std::string& message);
};

#endif
