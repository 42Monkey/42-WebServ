#ifndef PARSER_HPP
# define PARSER_HPP

# include "Lexer.hpp"
# include "ServerConfig.hpp"
# include "LocationConfig.hpp"

# include <vector>
# include <string>
# include <cstdlib>

class Parser {
	public :
		// const ref to the vector of tokens
		Parser(const std::vector<Lexer::Token>& token);

		// main parsing method
		// returns a vector of all parsed ServerConfig blocks
		std::vector<ServerConfig> parse();

	private :
		const std::vector<Lexer::Token>& _tokens;
		size_t _currentTokenIndex;

		// navigation methods
		const Lexer::Token& currentToken() const;
		void advance();
		bool isEOF() const;

		// error handling
		void throwParseError(const std::string &message) const;

		// consume methods
		void consume(Lexer::TokenType expectedType, const std::string &message);
		void skipComments();

		// parsing method for entire config file
		std::vector<ServerConfig> parseConfigFile();

		// parsing methods for blocks
		ServerConfig	parseServerBlock();
		LocationConfig	parseLocationBlock();

		// Server directive parsers
		void parseServerDirective(ServerConfig& server);
		void parseListenDirective(ServerConfig& server);
		void parseServerNameDirective(ServerConfig& server);
		void parseErrorPageDirective(ServerConfig& server);
		void parseClientMaxBodySizeDirective(ServerConfig& server);
		void parseHostDirective(ServerConfig& server);

		// Location directive parsers
		void parseLocationDirective(LocationConfig &location);
		void parseRootDirective(LocationConfig &location);
		void parseIndexDirective(LocationConfig &location);
		void parseMethodsDirective(LocationConfig &location);
		void parseAutoindexDirective(LocationConfig &location);
		void parseUploadStoreDirective(LocationConfig &location);
		void parseReturnDirective(LocationConfig &location);
		void parseCgiPassDirective(LocationConfig &location);
		void parseCgiExtensionDirective(LocationConfig &location);
		void parseClientMaxBodySizeDirective(LocationConfig &location);

		// validator
		void validateServerConfigs(const std::vector<ServerConfig> &config) const;
		void checkDuplicatePorts(const ServerConfig &server) const;
		void checkDuplicatePaths(const std::vector<LocationConfig> &locations) const;
		void checkDuplicateServerNames(const std::vector<ServerConfig> &servers) const;

		// set defaults
		void setServerDefault(ServerConfig& server) const;
		void setLocationDefault(LocationConfig& location) const;

		// utils
		void parseClientMaxBodySize(size_t &configSize, size_t maxSize);
		int parsePort(const std::string &string) const;
		int parseStatusCode(const std::string &string) const;
		bool parseBool(const std::string &value) const;
		void checkPath(std::string &path) const;
		void checkCgi(const LocationConfig &location) const;
		void checkBraces() const;
};

#endif

/*
 * the parser takes the vector of tokens from the lexer
 * verifies if they form valid syntax
 * populates ServerConfig structure
 *
 * parse() is the entry point
 * initiates parsing of server blocks
 * expect 'server' keyword followed by a '{'

 * dispatcher
 * dispatches to directive parser
 * recognizes the TOKEN_something and uses correct directive method

 * inside appropriate directive method
 * consumes the TOKEN
 * parser advances token pointer to next token
 * if no matches, throws syntax error

 * after processing
 * update the relevant ServerConfig object
 * parser continues to the next token, to the next directive in the server block

 * must ensure tokens are in the right order and structure
 * before our webserv attempts to use it
 */

