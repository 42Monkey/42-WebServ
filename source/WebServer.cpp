#include "../include/WebServer.hpp"
#include "../include/Multiplexer.hpp"
#include "../include/ServerConfig.hpp"
#include "../include/Request.hpp"
#include "../include/Response.hpp"
#include "../include/Router.hpp"
#include "../include/CGI.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/stat.h>

WebServer::WebServer(const std::vector<ServerConfig>& configs)
	: _configs(configs), _multiplexer(NULL), _router(NULL) {
	_multiplexer = new Multiplexer(this);
	_router = new Router(_configs);
	_initErrorPage();
}

WebServer::~WebServer() {
	for (size_t i = 0; i < _serverSockets.size(); ++i){
		close(_serverSockets[i]->getFd());
		delete _serverSockets[i];
	}
	_serverSockets.clear();

	delete _multiplexer;
	delete _router;
}

bool WebServer::initialize() {
	if (_configs.empty()) {
		std::cerr << "No server configurations" << std::endl;
		return false;
	}

	for (size_t i = 0; i < _configs.size(); ++i) {
		const ServerConfig& config = _configs[i];
		for (size_t j = 0; j < config._listenPorts.size(); ++j) {
			int port = config._listenPorts[j];
			std::string host = config.getHost();

			Socket* serverSocket = new Socket();
			if (!serverSocket->createSocket() ||
				!serverSocket->bindSocket(host, port) ||
				!serverSocket->listenSocket(10) ||
				!serverSocket->setNonBlocking()) {
				delete serverSocket;
				return false;
			}

			int server_fd = serverSocket->getFd();
			_serverSockets.push_back(serverSocket);
			_serverFdToConfig[server_fd] = i;

			if (!_multiplexer->addServerSocket(server_fd)) {
				std::cerr << "Failed to register server socket in multiplexer" << std::endl;
				return false;
			}

			std::cout << "Server initialized on http://" << host << ":" << port << std::endl;
		}
	}
	std::cout << ">>>> http://localhost:8080 <<<<" <<std::endl;
	return true;
}

void WebServer::run() {
	_multiplexer->run();
}

size_t WebServer::getConfigIndex(int server_fd) const {
	std::map<int, size_t>::const_iterator it = _serverFdToConfig.find(server_fd);
	if (it != _serverFdToConfig.end()) return it->second;
	return 0;
}

void WebServer::processRequest(int client_fd, const std::string& raw) {
	try {
		Request req(client_fd, raw);

		size_t cfgIndex = getConfigIndexFromClient(client_fd);
		const ServerConfig* server = &_configs[cfgIndex];

		const LocationConfig* location = _router->getLocationConfig(server, req.getPath());

		if (!server || !location) {
			_sendError(client_fd, 404, "Not Found");
			return;
		}

		const std::set<std::string>& methods = location->getAllowedMethods();
		std::string requestMethod = req.getMethod();

		bool methodAllowed = false;
		if (methods.empty()) {
			methodAllowed = true;
		} else {
			if (methods.count(requestMethod)) {
				methodAllowed = true;
			}
		}

		if (!methodAllowed) {
			_sendError(client_fd, 405, "Method Not Allowed");
			return;
		}

		if (req.getMethod() == "GET")
			_handleGet(req, *location, *server);
		else if (req.getMethod() == "POST")
			_handlePost(req, *location, *server);
		else if (req.getMethod() == "DELETE")
			_handleDelete(req, *location, *server);
		else
			_sendError(client_fd, 405, "Method Not Allowed");

	}
	catch (const std::exception& e) {
		std::cerr << "processRequest error: " << e.what() << std::endl;
		_sendError(client_fd, 400, "Bad Request");
	}
}

// Private
void WebServer::_handleGet(const Request& request, const LocationConfig& location, const ServerConfig& server) {
	int client_fd = request.getClientFd();

	if (_handleRedirect(client_fd, location)) {
		return;
	}

	std::string path;
	if (!handleTraversal(location._root, request.getPath().substr(location._path.size()), path)) {
		_sendError(client_fd, 400, "Bad Request : Traversal");
		return;
	}

	struct stat st;
	if (stat(path.c_str(), &st) != 0) {
		_sendError(client_fd, 404, "Not Found");
		return ;
	}

	if (S_ISDIR(st.st_mode)) {
		_handleDirectory(client_fd, path, request.getPath(), location);
		return;
	}
	if (S_ISREG(st.st_mode)) {
		if (location.isCgiEnabled()) {
			_handleCgi(client_fd, request, location, server);
		} else {
			if (!_multiplexer->startFileRead(client_fd, path)) {
				_sendError(client_fd, 500, "Failed to read file");
			}
		}
		return;
	}

	_sendError(client_fd, 403, "Forbidden");
}

void WebServer::_handlePost(const Request& request, const LocationConfig& location, const ServerConfig& server) {
	int client_fd = request.getClientFd();
	const std::string& uploadDirectory = location.getUploadStore();

	if (!_checkDirectory(client_fd, uploadDirectory)) {
		return;
	}

	if (location.isCgiEnabled()) {
		_handleCgi(client_fd, request, location, server);
		return;
	}

	std::string normalizedUploadDir = uploadDirectory;
	if (!normalizedUploadDir.empty() && normalizedUploadDir[normalizedUploadDir.size() - 1] != '/') {
		normalizedUploadDir += "/";
	}

	std::string originalFilename = _extractFilename(request);
	if (originalFilename.empty()) {
		_sendError(client_fd, 400, "No filename provided in request");
		return;
	}

	std::string finalFilename = _resolveFilename(normalizedUploadDir, originalFilename);
	std::string fullPath = normalizedUploadDir + finalFilename;

	// Use async file write instead of synchronous _saveFile
	if (!_multiplexer->startFileWrite(client_fd, fullPath, request.getBody())) {
		_sendError(client_fd, 500, "Failed to save file");
		return;
	}
}

// Keep the rest of the WebServer methods unchanged, but add this helper:
bool WebServer::_isAsyncFileOperation(const std::string& path) const {
	struct stat st;
	if (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
		return true;
	}
	return false;
}

void WebServer::_handleDelete(const Request& request, const LocationConfig& location, const ServerConfig& server) {
	int client_fd = request.getClientFd();

	if (location.isCgiEnabled()) {
		_handleCgi(client_fd, request, location, server);
		return;
	}

	std::string filename;

	std::string query = request.getQuery();
	size_t pos = query.find("file=");
	if (pos != std::string::npos) {
		filename = query.substr(pos + 5);
	}

	if (filename.empty()) {
		_sendError(client_fd, 400, "No file specified");
		return;
	}

	std::string uploadDir = location.getUploadStore();
	if (uploadDir.empty()) {
		_sendError(client_fd, 403, "Upload directory not configured");
		return;
	}

	if (!uploadDir.empty() && uploadDir[uploadDir.size() - 1] != '/')
		uploadDir += "/";

	std::string filePath = uploadDir + filename;

	struct stat st;
	if (stat(filePath.c_str(), &st) != 0) {
		_sendError(client_fd, 404, "File not found or cannot delete");
		return;
	}

	if (S_ISDIR(st.st_mode)) {
		_sendError(client_fd, 409, "Cannot delete a directory");
		return;
	}

	if (unlink(filePath.c_str()) != 0) {
		_sendError(client_fd, 500, "Failed to delete file");
		return;
	}

	Response res;
	res.setStatus(200, "OK");
	res.setBody("<h1>File deleted successfully: " + filename + "</h1>");
	_sendResponse(client_fd, res);
}

void WebServer::_handleCgi(int client_fd, const Request& request, const LocationConfig& location, const ServerConfig& server)
{
	CGI* cgi = new CGI(request, location, server);

	// First, check if the CGI interpreter is configured
	std::string interpreterPath = location.getCgiPath();
	if (interpreterPath.empty()) {
		_sendError(client_fd, 500, "CGI interpreter not configured");
		delete cgi;
		return;
	}

	// Check if the interpreter exists and is executable
	struct stat interpreter_st;
	if (stat(interpreterPath.c_str(), &interpreter_st) != 0) {
		_sendError(client_fd, 500, "CGI interpreter not found");
		delete cgi;
		return;
	}

	if (!S_ISREG(interpreter_st.st_mode)) {
		_sendError(client_fd, 500, "CGI interpreter is not a regular file");
		delete cgi;
		return;
	}

	if ((interpreter_st.st_mode & S_IXUSR) == 0) {
		_sendError(client_fd, 500, "CGI interpreter is not executable");
		delete cgi;
		return;
	}

	// Now check the actual script file that was requested
	std::string scriptPath = location._root + request.getPath().substr(location._path.length());

	struct stat script_st;
	if (stat(scriptPath.c_str(), &script_st) != 0) {
		_sendError(client_fd, 404, "CGI script not found");
		delete cgi;
		return;
	}

	if (!S_ISREG(script_st.st_mode)) {
		_sendError(client_fd, 500, "CGI script is not a regular file");
		delete cgi;
		return;
	}

	if ((script_st.st_mode & S_IRUSR) == 0) {
		_sendError(client_fd, 403, "CGI script is not readable");
		delete cgi;
		return;
	}

	int pipe_fd = cgi->executeAsync(client_fd);

	if (pipe_fd < 0) {
		Response res;
		res.setStatus(500, "Internal Server Error");
		res.setBody("<h1>Failed to execute CGI</h1>");
		_sendResponse(client_fd, res);
		delete cgi;
		return;
	}

	_multiplexer->addCgi(client_fd, pipe_fd, cgi);
}

void WebServer::_sendResponse(int client_fd, const Response& response) {
	std::ostringstream oss;
	oss << response.toString();
	_multiplexer->queueResponse(client_fd, oss.str());
}

size_t WebServer::_getMaxBodySizeForClient(int client_fd) const {
	size_t configIndex = getConfigIndex(client_fd);
	const ServerConfig& config = _configs[configIndex];

	return config._clientMaxBodySize;
}

// off_t describes file sizes, size_t is for objects
std::string WebServer::_formatFileSize(off_t size) {
	if (size < 0) {
		return "Error";
	}

	// (B)
	if (size < 1024) {
		std::ostringstream ss;
		ss << size << " B";
		return ss.str();
	}

	double dSize = static_cast<double>(size);
	std::ostringstream ss;

	ss << std::fixed << std::setprecision(1);

	// (KB)
	if (dSize < 1024.0 * 1024.0) {
		dSize /= 1024.0;
		ss << dSize << " KB";
		return ss.str();
	}

	// (MB)
	if (dSize < 1024.0 * 1024.0 * 1024.0) {
		dSize /= (1024.0 * 1024.0);
		ss << dSize << " MB";
		return ss.str();
	}

	dSize /= (1024.0 * 1024.0 * 1024.0);
	ss << dSize << " GB";
	return ss.str();
}

std::string WebServer::_buildEntryLink(const std::string& requestPath, const std::string& name, bool isDirectory) {
	std::string linkPath = requestPath;

	if (linkPath.empty() || linkPath[linkPath.length() - 1] != '/') {
		linkPath += "/";
	}
	linkPath += name;

	if (isDirectory) {
		linkPath += "/";
	}

	return linkPath;
}

std::string WebServer::_formatTimestamp(time_t rawTime) {
	char buffer[80];
	struct tm *tm_local = localtime(&rawTime);
	if (tm_local == NULL) {
		return "Unknown Date";
	}
	// Format: YYYY-MM-DD HH:MM:SS
	if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_local) == 0) {
		return "Formatting Error";
	}
	return buffer;
}

std::string WebServer::_generateDirectoryHTML(const std::string& path, const std::string& requestPath) {
	std::ostringstream html;

	html << "<html><head><title>Index of " << requestPath << "</title></head><body>\n"
	<< "<h1>Index of " << requestPath << "</h1>\n"
	<< "<hr>\n"
	<< "<table style=\"width:100%; border-collapse: collapse;\">\n"
	// Table Header
	<< "<tr>"
	<< "<th style=\"text-align:left; border-bottom: 1px solid #ccc; padding: 5px 0;\">Name</th>"
	<< "<th style=\"text-align:left; border-bottom: 1px solid #ccc; padding: 5px 0; width: 200px;\">Last Modified</th>"
	<< "<th style=\"text-align:right; border-bottom: 1px solid #ccc; padding: 5px 0; width: 100px;\">Size</th>"
	<< "</tr>\n";


	// Parent Directory Link (..)
	if (requestPath != "/") {
		std::string parentPath;
		size_t lastSlash = requestPath.find_last_of('/');
		if (lastSlash == 0) {
			parentPath = "/";
		} else {
			// Find the slash before the last one, robustly
			size_t parentEnd = requestPath.length() - 1;
			if (requestPath[parentEnd] == '/') {
				parentEnd--;
			}
			lastSlash = requestPath.substr(0, parentEnd).find_last_of('/');
			if (lastSlash == std::string::npos) {
				parentPath = "/";
			} else {
				parentPath = requestPath.substr(0, lastSlash + 1);
			}
		}

		html << "<tr>"
			 << "<td style=\"padding: 5px 0;\"><a href=\"" << parentPath << "\">..</a></td>"
			 << "<td style=\"padding: 5px 0;\"></td>"
			 << "<td style=\"text-align:right; padding: 5px 0;\">[Parent Dir]</td>"
			 << "</tr>\n";
	}

	DIR* dir = opendir(path.c_str());
	if (!dir) {
		html << "<tr><td colspan=\"3\" style=\"color: red;\">Error reading directory.</td></tr>\n";
	} else {
		struct dirent* entry;
		while ((entry = readdir(dir)) != NULL) {
			std::string name = entry->d_name;

			if (name == "." || name == "..") {
				continue;
			}
			if (name[0] == '.') {
				continue;
			}

			std::string fullPath = path + "/" + name;
			struct stat st;
			bool isDirectory = false;
			std::string size = "-";
			std::string timestamp = "N/A";

			if (stat(fullPath.c_str(), &st) == 0) {
				// Get timestamp
				timestamp = _formatTimestamp(st.st_mtime);

				if (S_ISDIR(st.st_mode)) {
					isDirectory = true;
					size = "[Directory]";
				} else if (S_ISREG(st.st_mode)) {
					size = _formatFileSize(st.st_size);
				} else {
					size = "[Other]";
				}
			}

			std::string linkPath = _buildEntryLink(requestPath, name, isDirectory);

			// Output table row
			html << "<tr>"
				 << "<td style=\"padding: 5px 0;\"><a href=\"" << linkPath << "\">" << name << (isDirectory ? "/" : "") << "</a></td>"
				 << "<td style=\"padding: 5px 0;\">" << timestamp << "</td>"
				 << "<td style=\"text-align:right; padding: 5px 0;\">" << size << "</td>"
				 << "</tr>\n";
		}
		closedir(dir);
	}

	html << "</table>\n"
	<< "<hr>\n"
	<< "</body></html>\n";
	return html.str();
}

// try to server index file
// if no index file, check autoindex
// Updated _handleDirectory method to use async file operations
bool WebServer::_handleDirectory(int client_fd, const std::string& directoryPath, const std::string& requestPath, const LocationConfig& location) {
	// Check for index file first
	if (!location._index.empty()) {
		std::string indexPath = directoryPath;

		if (indexPath[indexPath.length() - 1] != '/') {
			indexPath += "/";
		}
		indexPath += location.getIndex();

		struct stat index_st;
		if (stat(indexPath.c_str(), &index_st) == 0 && S_ISREG(index_st.st_mode)) {
			// Use async file read instead of synchronous _serveFile
			if (!_multiplexer->startFileRead(client_fd, indexPath)) {
				_sendError(client_fd, 500, "Failed to read index file");
				return false;
			}
			return true;
		}
	}

	// If no index file found, check autoindex
	if (location.getAutoindex()) {
		DIR* testDir = opendir(directoryPath.c_str());
		if (!testDir) {
			if (errno == EACCES) {
				_sendError(client_fd, 403, "Permission Denied");
			} else {
				_sendError(client_fd, 404, "Directory Not Found");
			}
			return false;
		}
		closedir(testDir);

		std::string html = _generateDirectoryHTML(directoryPath, requestPath);

		Response response;
		response.setStatus(200, "OK");
		response.setHeader("Content-Type", "text/html; charset=utf-8");
		response.setHeader("Cache-Control", "no-cache, no-store, must-revalidate");
		response.setHeader("Pragma", "no-cache");
		response.setHeader("Expires", "0");
		response.setBody(html);

		_sendResponse(client_fd, response);
		return true;
	} else {
		_sendError(client_fd, 403, "403 : Forbidden _handleDirectory");
		return false;
	}
}

bool WebServer::_checkDirectory(int client_fd, const std::string& path) {
	if (path.empty()) {
		_sendError(client_fd, 500, "Internal Server Error: Upload store directory path is not configured.");
		return false;
	}

	struct stat st;
	if (stat(path.c_str(), &st) != 0) {
		if (mkdir(path.c_str(), 0777) == -1) {
			if (errno == EACCES || errno == EROFS)
				_sendError(client_fd, 403, "Forbidden: Server lacks permission to create directory on the file system.");
			else
				_sendError(client_fd, 500, "Internal Error: Failed to create upload directory due to a system error.");
			return false;
		}
	} else if (!S_ISDIR(st.st_mode)) {
		_sendError(client_fd, 500, "Internal Error: Configured path exists but is not a directory.");
		return false;
	}
	return true;
}

// front-end uses
// x-file-extension
// x-filename
std::string WebServer::_extractFilename(const Request& request) {
	// std::cout << "=== DEBUG: All Headers ===" << std::endl;
	// const std::map<std::string, std::string>& headers = request.getHeaders();
	// for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
	//     std::cout << it->first << ": " << it->second << std::endl;
	// }
	// std::cout << "=== END DEBUG ===" << std::endl;

	std::string xFilename = request.getHeader("x-filename");
	if (!xFilename.empty()) {
		return xFilename;
	}
	return "default";
}

std::string WebServer::_resolveFilename(const std::string& uploadDir, const std::string& filename) {
	std::string finalFilename = filename;
	std::string fullPath = uploadDir + finalFilename;
	int count = 1;

	struct stat st;
	while (stat(fullPath.c_str(), &st) == 0) {
		size_t dot = filename.find_last_of('.');

		if (dot != std::string::npos) {
			std::string name = filename.substr(0, dot);
			std::string ext = filename.substr(dot);
			std::ostringstream newName;
			newName << name << "(" << count << ")" << ext;
			finalFilename = newName.str();
		} else {
			std::ostringstream newName;
			newName << filename << "(" << count << ")";
			finalFilename = newName.str();
		}
		fullPath = uploadDir + finalFilename;
		count++;
	}
	return finalFilename;
}

bool WebServer::_handleRedirect(int client_fd, const LocationConfig& location) {
	if (location._redirectCode.empty()) {
		return false;
	}

	int code = std::atoi(location._redirectCode.c_str());
	if (code < 300 || code >= 400) {
		code = 301;
	}

	std::string reason;
	if (code == 301) {
		reason = "Moved permanently";
	} else if (code == 302) {
		reason = "Found";
	} else {
		reason = "Redirect";
	}

	Response response;
	response.setStatus(code, reason);
	response.setHeader("Location", location._redirectUrl);
	_sendResponse(client_fd, response);
	return true;
}

// must not handle directory as cgi request
// check if path has a cgi file extension
bool WebServer::_isCgiRequest(const std::string& path, const LocationConfig& location) {
	if (!path.empty() && path[path.length() - 1] == '/') {
		return false;
	}

	size_t dot = path.find_last_of('.');
	if (dot != std::string::npos) {
		std::string ext = path.substr(dot + 1);
		const std::vector<std::string>& cgi_extensions = location.getCgiExtensions();

		for (size_t i = 0; i < cgi_extensions.size(); ++i) {
			if (cgi_extensions[i] == ext) {
				return true;
			}
		}
	}
	return false;
}

size_t WebServer::getConfigIndexFromClient(int client_fd) const {
	int server_fd = _multiplexer->getServerFdForClient(client_fd);
	return getConfigIndex(server_fd);
}

void WebServer::_sendError(int client_fd, int code, const std::string& message) {
	Response res;
	res.setStatus(code, message);
	res.setHeader("Content-Type", "text/html; charset=utf=8");
	res.setHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	res.setHeader("Connection", "close");

	std::map<int, std::string>::const_iterator it = _errorPage.find(code);
	if (it != _errorPage.end()) {
		res.setBody(it->second);
	} else {
		res.setBody(_generateErrorPage(code, message));
	}
	_sendResponse(client_fd, res);
}

void WebServer::_initErrorPage() {
	_errorPage[400] = _generateErrorPage(400, "Bad Request");
	_errorPage[403] = _generateErrorPage(403, "Forbidden");
	_errorPage[404] = _generateErrorPage(404, "Not Found");
	_errorPage[405] = _generateErrorPage(405, "Method Not Allowed");
	_errorPage[409] = _generateErrorPage(409, "Conflict");
	_errorPage[413] = _generateErrorPage(413, "Request Entity Too Large");
	_errorPage[500] = _generateErrorPage(500, "Internal Server Error");
	_errorPage[502] = _generateErrorPage(502, "Bad Gateway");
}

std::string WebServer::_generateErrorPage(int code, const std::string& message) {
	std::ostringstream html;

	html << "<!DOCTYPE html>\n"
	<< "<html lang=\"en\">\n"
	<< "<head>\n"
	<< "    <meta charset=\"UTF-8\">\n"
	<< "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
	<< "    <title>" << code << " " << message << "</title>\n"
	<< "    <style>\n"
	<< "        body { font-family: Arial, sans-serif; text-align: center; padding: 50px; }\n"
	<< "        h1 { font-size: 50px; color: #e74c3c; }\n"
	<< "        p { font-size: 20px; color: #555; }\n"
	<< "    </style>\n"
	<< "</head>\n"
	<< "<body>\n"
	<< "    <h1>" << code << " " << message << "</h1>\n"
	<< "    <p>Something went wrong. Please try again later.</p>\n"
	<< "</body>\n"
	<< "</html>";

	return html.str();
}

// if user path is empty, use root
// combines paths
// checks for ".."
bool WebServer::handleTraversal(const std::string& root, const std::string& user, std::string& path) {
	if (user.empty() || user == "/") {
		path = root.empty() ? "/" : root;
		return true;
	}

	std::string combined;
	if (root.empty()) {
		combined = user;
	} else if (root[root.size() - 1] == '/' && user[0] == '/') {
		combined = root + user.substr(1);
	} else if (root[root.size() - 1] != '/' && user[0] != '/') {
		combined = root + "/" + user;
	} else {
		combined = root + user;
	}

	if (combined.find("/../") != std::string::npos ||
		combined.find("..") == 0 ||
		combined.rfind("/..") == combined.length() - 3) {
		return false;
	}

	path = combined;
	return true;
}
