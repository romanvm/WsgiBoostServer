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
    """Simplest possible application object"""
    text = 'Hello World!\nI love you!\n\n'
    text += 'POST data: {0}\n\n'.format(environ['wsgi.input'].read())
    text += 'environ variables:\n'
    for key, value in environ.iteritems():
        text += '{0}: {1}\n'.format(key, value)
    status = '200 OK'
    content = StringIO(text)
    response_headers = [('Content-type', 'text/plain'), ('Content-Length', str(len(text)))]
    start_response(status, response_headers)
    return content


print('Startig WsgiBoost server...')
httpd =  wsgi_boost.WsgiBoostHttp(8000, 10)
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