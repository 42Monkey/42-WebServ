#include "../include/Socket.hpp"
#include "../include/Lexer.hpp"
#include "../include/Parser.hpp"
#include "../include/ServerConfig.hpp"
#include "../include/WebServer.hpp"
#include "../include/CGI.hpp"
#include "../include/Multiplexer.hpp"
#include "../include/Router.hpp"
#include "../include/Response.hpp"
#include "../include/Request.hpp"
#include <fstream>
#include <iostream>
#include <sys/stat.h>

bool isGoodFile(const std::string& path) {
	struct stat pathStat;

	if (stat(path.c_str(), &pathStat) != 0) {
		return false;
	}
	return S_ISREG(pathStat.st_mode);
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file_path>" << std::endl;
		return 1;
	}

	const std::string config_file_path = argv[1];

	if (!isGoodFile(config_file_path)) {
		std::cerr << "Error: " << config_file_path << "is not good" << std::endl;
		return 1;
	}

	std::ifstream config_file(config_file_path.c_str());
	if (!config_file.is_open()) {
		std::cerr << "Error: Could not open configuration file '" << config_file_path << "'" << std::endl;
		return 1;
	}
	std::string config_string((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());
	config_file.close();

	Lexer lexer(config_string);
	std::vector<Lexer::Token> tokens = lexer.tokenize();

	Parser parser(tokens);
	std::vector<ServerConfig> configs;
	try {
		configs = parser.parse();
	} catch (const std::exception& e) {
		std::cerr << "Config parse error: " << e.what() << std::endl;
		return 1;
	}

	WebServer server(configs);
	if (!server.initialize())
	{
		std::cerr << "Failed to initialize server" << std::endl;
		return (1);
	}

	std::cout << "Starting server..." << std::endl;
	server.run();
	return (0);
}
