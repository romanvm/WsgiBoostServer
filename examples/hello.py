#!/usr/bin/env python

import os
import sys

cwd = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(cwd))

import wsgi_boost


def hello_app(environ, start_response):
    content = 'Hello World!'
    response_headers = [('Content-type', 'text/plain'), ('Content-Length', str(len(content)))]
    start_response('200 OK', response_headers)
    return [content]


httpd = wsgi_boost.WsgiBoostHttp(num_threads=8)
httpd.set_app(hello_app)
httpd.start()
