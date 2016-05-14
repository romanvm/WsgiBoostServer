#!/usr/bin/env python
# coding: utf-8

from __future__ import print_function
import os
import sys

cwd = os.path.dirname(os.path.abspath(__file__))
# wsgi_boost_dir = os.path.join(os.path.dirname(cwd), 'wsgi_boost')
# sys.path.insert(0, wsgi_boost_dir)

import wsgi_boost


def simple_app(environ, start_response):
    content = 'Hello World!\n\n'
    content += 'POST data:\n\n'
    for line in environ['wsgi.input']:
        print("line: " + line)
        content += line
    content += 'environ variables:\n'
    for key, value in environ.iteritems():
        content += '{0}: {1}\n'.format(key, value)
    status = '200 OK'
    response_headers = [('Content-type', 'text/plain'), ('Content-Length', str(len(content)))]
    start_response(status, response_headers)
    return [content]


print('Startig WsgiBoost server...')
httpd =  wsgi_boost.WsgiBoostHttp(num_threads=4)
httpd.add_static_route('^/static', cwd)
httpd.set_app(simple_app)
httpd.start()
print('WsgiBoost server stopped.')
