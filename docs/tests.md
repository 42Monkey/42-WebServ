# WebServer Testing Commands

## Telnet Manual Tests

### Basic HTTP Requests & Server Rules
```bash
# Basic GET Request
telnet localhost 8080
GET / HTTP/1.1
Host: localhost:8080

# Directory Listing (Autoindex)
telnet localhost 8080
GET /upload/ HTTP/1.1
Host: localhost:8080

# Serve Static File
telnet localhost 8080
GET /index.html HTTP/1.1
Host: localhost:8080

# 404 Not Found
telnet localhost 8080
GET /nonexistent.html HTTP/1.1
Host: localhost:8080

# Redirection
telnet localhost 8080
GET /redirect HTTP/1.1
Host: localhost:8080

# 405 Method Not Allowed
telnet localhost 8080
PUT /index.html HTTP/1.1
Host: localhost:8080

# 400 Missing Host Header
telnet localhost 8080
GET / HTTP/1.1

# 400 Invalid Syntax
telnet localhost 8080
INVALID REQUEST
```

### Multi-Port & Max Body Limits
```bash
# Different Server Port
telnet localhost 9081
GET / HTTP/1.1
Host: localhost:9081

# 413 Payload Too Large (client_max_body_size)
telnet localhost 9091
POST / HTTP/1.1
Host: localhost:9091
Content-Length: 1000

# [Paste 1000 bytes - triggers 413 error]
```
### File Operations

```bash
# POST Upload (Custom Header)
telnet localhost 8080
POST /upload HTTP/1.1
Host: localhost:8080
Content-Length: 12
x-filename: test.txt

Hello World!

# DELETE File
telnet localhost 8080
DELETE /upload?file=test.txt HTTP/1.1
Host: localhost:8080
```

### CGI Handler Execution
```bash
# Valid CGI Script
telnet localhost 8080
GET /cgi-bin/test.php HTTP/1.1
Host: localhost:8080

# Nonexistent Script (Expect 500)
telnet localhost 8080
GET /cgi-bin/nonexistent.php HTTP/1.1
Host: localhost:8080

# CGI Directory Target (Expect 404/500)
telnet localhost 8080
GET /cgi-bin/ HTTP/1.1
Host: localhost:8080
```


### cURL Commands
```bash
# Basic HTTP Operations
curl -X GET http://localhost:8080/
curl -X POST http://localhost:8080/upload -d "test data"
curl -v -F "file=@testfile.txt" http://localhost:8080/upload
curl -X DELETE "http://localhost:8080/upload?file=testfile.txt"

# CGI Tests
curl -v http://localhost:8080/cgi-bin/test.php
curl -v -X POST http://localhost:8080/cgi-bin/test.php -d "data=test"

# Error Conditions
curl -v http://localhost:8080/nonexistent
curl -v -X PUT http://localhost:8080/
```

### Load Testing
```bash
# Siege (10 concurrent clients for 30 seconds)
siege -c 10 -t 30s http://localhost:8080/

# ApacheBench (100 total requests, concurrency 10)
ab -n 100 -c 10 http://localhost:8080/
```
