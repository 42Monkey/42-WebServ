#include "../include/Constants.hpp"

std::string getReasonPhrase(int statusCode) {
	struct StatusPhrase {
		int code;
		const char* phrase;
	};

	static const StatusPhrase phrases[] = {
		// 1xx Informational
		{HTTP_CONTINUE, "Continue"},
		{HTTP_SWITCHING_PROTOCOLS, "Switching Protocols"},

		// 2xx Success
		{HTTP_OK, "OK"},
		{HTTP_CREATED, "Created"},
		{HTTP_ACCEPTED, "Accepted"},
		{HTTP_NON_AUTHORITATIVE_INFO, "Non-Authoritative Information"},
		{HTTP_NO_CONTENT, "No Content"},
		{HTTP_RESET_CONTENT, "Reset Content"},
		{HTTP_PARTIAL_CONTENT, "Partial Content"},

		// 3xx Redirection
		{HTTP_MULTIPLE_CHOICES, "Multiple Choices"},
		{HTTP_MOVED_PERMANENTLY, "Moved Permanently"},
		{HTTP_FOUND, "Found"},
		{HTTP_SEE_OTHER, "See Other"},
		{HTTP_NOT_MODIFIED, "Not Modified"},
		{HTTP_USE_PROXY, "Use Proxy"},
		{HTTP_TEMPORARY_REDIRECT, "Temporary Redirect"},

		// 4xx Client Errors
		{HTTP_BAD_REQUEST, "Bad Request"},
		{HTTP_UNAUTHORIZED, "Unauthorized"},
		{HTTP_PAYMENT_REQUIRED, "Payment Required"},
		{HTTP_FORBIDDEN, "Forbidden"},
		{HTTP_NOT_FOUND, "Not Found"},
		{HTTP_METHOD_NOT_ALLOWED, "Method Not Allowed"},
		{HTTP_NOT_ACCEPTABLE, "Not Acceptable"},
		{HTTP_PROXY_AUTH_REQUIRED, "Proxy Authentication Required"},
		{HTTP_REQUEST_TIMEOUT, "Request Timeout"},
		{HTTP_CONFLICT, "Conflict"},
		{HTTP_GONE, "Gone"},
		{HTTP_LENGTH_REQUIRED, "Length Required"},
		{HTTP_PRECONDITION_FAILED, "Precondition Failed"},
		{HTTP_PAYLOAD_TOO_LARGE, "Payload Too Large"},
		{HTTP_URI_TOO_LONG, "URI Too Long"},
		{HTTP_UNSUPPORTED_MEDIA_TYPE, "Unsupported Media Type"},
		{HTTP_RANGE_NOT_SATISFIABLE, "Range Not Satisfiable"},
		{HTTP_EXPECTATION_FAILED, "Expectation Failed"},

		// 5xx Server Errors
		{HTTP_INTERNAL_SERVER_ERROR, "Internal Server Error"},
		{HTTP_NOT_IMPLEMENTED, "Not Implemented"},
		{HTTP_BAD_GATEWAY, "Bad Gateway"},
		{HTTP_SERVICE_UNAVAILABLE, "Service Unavailable"},
		{HTTP_GATEWAY_TIMEOUT, "Gateway Timeout"},
		{HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported"},

		// Terminator
		{0, NULL}
	};

	for (int i = 0; phrases[i].phrase != NULL; ++i) {
		if (phrases[i].code == statusCode) {
			return phrases[i].phrase;
		}
	}
	return "Unknown Status";
}
