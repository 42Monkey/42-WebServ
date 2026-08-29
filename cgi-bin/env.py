#!/usr/bin/env python3
import os
print("Content-Type: text/html\n")
print("<html><body>")
print("<h1>Environment Variables Test</h1>")
print("<table border='1'>")
env_vars = [
    'REQUEST_METHOD', 'QUERY_STRING', 'CONTENT_TYPE', 'CONTENT_LENGTH',
    'HTTP_USER_AGENT', 'HTTP_HOST', 'SERVER_NAME', 'SERVER_PORT',
    'SCRIPT_NAME', 'PATH_INFO', 'REMOTE_ADDR', 'HTTP_ACCEPT'
]
for var in env_vars:
    value = os.environ.get(var, 'Not set')
    print(f"<tr><td><b>{var}</b></td><td>{value}</td></tr>")
print("</table>")
print("</body></html>")
