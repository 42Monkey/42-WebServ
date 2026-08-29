#!/usr/bin/env python3
import os
import sys
import json

method = os.environ.get('REQUEST_METHOD', 'GET')
content_length = int(os.environ.get('CONTENT_LENGTH', '0'))

print("Content-Type: application/json\n")

response = {
    'method': method,
    'query_string': os.environ.get('QUERY_STRING', ''),
    'content_type': os.environ.get('CONTENT_TYPE', ''),
    'content_length': content_length,
    'headers': {},
    'body': ''
}

# Collect HTTP headers
for key, value in os.environ.items():
    if key.startswith('HTTP_'):
        header_name = key[5:].replace('_', '-').title()
        response['headers'][header_name] = value

# Read body if present
if content_length > 0:
    response['body'] = sys.stdin.read(content_length)

print(json.dumps(response, indent=2))
