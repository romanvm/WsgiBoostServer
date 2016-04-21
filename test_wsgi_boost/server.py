#!/usr/bin/env python
# coding: utf-8

from __future__ import print_function
import os
import sys
import threading
import time

cwd = os.path.dirname(os.path.abspath(__file__))
wsgi_boost_dir = os.path.join(os.path.dirname(cwd), 'wsgi_boost')
sys.path.insert(0, wsgi_boost_dir)

import wsgi_boost

print('Startig WsgiBoost server...')
httpd =  wsgi_boost.WsgiBoostHttp(8000, 8)
httpd.add_static_route('^/static', cwd)
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
