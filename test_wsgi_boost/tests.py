#!/usr/bin/env python
# coding: utf-8

from __future__ import print_function
import os
import sys
import threading
import time

print('Running Python tests...')

cwd = os.path.dirname(os.path.abspath(__file__))
project_dir = os.path.dirname(cwd)
wsgi_boost_dir = os.path.join(project_dir, 'wsgi_boost')
sys.path.insert(0, wsgi_boost_dir)

import wsgi_boost

httpd = wsgi_boost.WsgiBoostHttp(8000, 8)
server_thread = threading.Thread(target=httpd.start)
server_thread.daemon = True
server_thread.start()
time.sleep(1.0)
httpd.stop()
server_thread.join()

print('All Python tests passed.')
