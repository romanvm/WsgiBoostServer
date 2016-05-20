#!/usr/bin/env python
# coding: utf-8

from __future__ import print_function
import os
import sys
import time
from wsgiref.validate import validator

cwd = os.path.dirname(os.path.abspath(__file__))
# wsgi_boost_dir = os.path.join(os.path.dirname(cwd), 'wsgi_boost')
# sys.path.insert(0, wsgi_boost_dir)

import wsgi_boost


def simple_app(environ, start_response):
    content = 'Hello World!\n\n'
    content += 'POST data:\n'
    for line in environ['wsgi.input']:
        content += line
    # content += environ['wsgi.input'].read()
    content += '\n\n'
    content += 'environ variables:\n'
    for key, value in environ.iteritems():
        content += '{0}: {1}\n'.format(key, value)
    status = '200 OK'
    response_headers = [('Content-type', 'text/plain'), ('Content-Length', str(len(content)))]
    start_response(status, response_headers)
    return [content]


httpd = wsgi_boost.WsgiBoostHttp(num_threads=4)
httpd.wsgi_debug = True
httpd.add_static_route('^/static', cwd)
httpd.set_app(simple_app)
httpd.start()
