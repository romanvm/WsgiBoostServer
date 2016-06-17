#!/usr/bin/env python
"""
Waitress benchmark application
"""

from waitress import serve


def hello_app(environ, start_response):
    content = b'Hello World!'
    response_headers = [('Content-type', 'text/plain'), ('Content-Length', str(len(content)))]
    start_response('200 OK', response_headers)
    return [content]


if __name__ == '__main__':
    serve(hello_app, host='0.0.0.0', port=8000)
