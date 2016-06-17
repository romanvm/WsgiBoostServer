#!/usr/bin/env python
"""
WsgiBoostServer benchmark
"""

import wsgi_boost


def hello_app(environ, start_response):
    content = b'Hello World!'
    response_headers = [('Content-type', 'text/plain'), ('Content-Length', str(len(content)))]
    start_response('200 OK', response_headers)
    return [content]


if __name__ == '__main__':
    httpd = wsgi_boost.WsgiBoostHttp(num_threads=10)
    httpd.set_app(hello_app)
    httpd.start()
