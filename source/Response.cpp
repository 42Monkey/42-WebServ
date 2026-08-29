#include "../include/Response.hpp"

Response::Response() : _statusCode(200), _statusMessage("OK") {
	_version = "HTTP/1.1";
	setHeader("Content-Type", "text/html");
}

Response::~Response() {}

void Response::setStatus(int code, const std::string &message) {
	if (code < 100 || code > 599) {
		std::cerr << "Invalid HTTP status code" << std::endl;
	}
	_statusCode = code;
	_statusMessage = message;
}

void Response::setHeader(const std::string &key, const std::string &value) {
	_headers[key] = value;
}

void Response::setBody(const std::string &body) {
	_body = body;

	std::ostringstream oss;
	oss << _body.size();
	setHeader("Content-Length", oss.str());
}

void Response::setContentType(const std::string &filePath) {
	std::string mimeType = _getMimeType(filePath);
	setHeader("Content-Type", mimeType);
}

std::string Response::toString() const {
	std::ostringstream response;

	response << _version << " " << _statusCode << " " << _statusMessage << "\r\n";

	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
		response << it->first << ": " << it->second << "\r\n";
	}

	response << "\r\n";
	response << _body;
	return response.str();
}


int Response::getStatusCode() const {
	return _statusCode;
}

const std::string& Response::getStatusMessage() const {
	return _statusMessage;
}

const std::string& Response::getVersion() const {
	return _version;
}

const std::map<std::string, std::string>& Response::getHeaders() const {
	return _headers;
}

const std::string& Response::getBody() const {
	return _body;
}

const std::string Response::_getMimeType(const std::string& filePath) const {
	std::map<std::string, std::string> mimeTypes;
	mimeTypes[".html"] = "text/html";
	mimeTypes[".css"] = "text/css";
	mimeTypes[".js"] = "application/javascript";
	mimeTypes[".json"] = "application/json";
	mimeTypes[".jpg"] = "image/jpeg";
	mimeTypes[".jpeg"] = "image/jpeg";
	mimeTypes[".png"] = "image/png";
	mimeTypes[".gif"] = "image/gif";
	mimeTypes[".txt"] = "text/plain";
	mimeTypes[".pdf"] = "application/pdf";
	mimeTypes[".mp4"] = "video/mp4";

	size_t dotPos = filePath.rfind('.');
	if (dotPos != std::string::npos) {
		std::string extension = filePath.substr(dotPos);
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
		std::map<std::string, std::string>::const_iterator it = mimeTypes.find(extension);
		if (it != mimeTypes.end()) {
			return it->second;
		}
	}
	return "application/octet-stream";
}
