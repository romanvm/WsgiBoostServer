#!/usr/bin/env python
# coding: utf-8

from __future__ import print_function
import os
import sys
import threading
import time
from wsgiref.validate import validator
from cStringIO import StringIO
from waitress import serve

cwd = os.path.dirname(os.path.abspath(__file__))
wsgi_boost_dir = os.path.join(os.path.dirname(cwd), 'wsgi_boost')
sys.path.insert(0, wsgi_boost_dir)

import wsgi_boost


def simple_app(environ, start_response):
    content = 'Hello World!\n\n'
    content += 'POST data: {0}\n\n'.format(environ['wsgi.input'].read())
    content += 'environ variables:\n'
    for key, value in environ.iteritems():
        content += '{0}: {1}\n'.format(key, value)
    status = '200 OK'
    response_headers = [('Content-type', 'text/plain'), ('Content-Length', str(len(content)))]
    start_response(status, response_headers)
    return [content]


print('Startig WsgiBoost server...')
httpd =  wsgi_boost.WsgiBoostHttp(8000, 4)
httpd.logging = True
httpd.add_static_route('^/static', cwd)
httpd.set_app(simple_app)
server_thread = threading.Thread(target=httpd.start)
server_thread.daemon = True
server_thread.start()
time.sleep(0.5)
print('WsgiBoost server started. To stop it press Ctrl+C.')
try:
    while True:
        time.sleep(0.1)
except KeyboardInterrupt:
    pass
finally:
    httpd.stop()
    server_thread.join()
print('WsgiBoost server stopped.')

# serve(simple_app, host='0.0.0.0', port=8000)