#!/usr/bin/env python
"""
This example demonstrates serving a Django application
that returns a rendered web-page with static content (Bootstrap CSS and a picture)
"""

import os
import sys
from wsgi_boost import WsgiBoostHttp

cwd = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(cwd, 'django_example'))

from django_example.wsgi import application

if __name__ == '__main__':
    # Create a server on the default port 8000 with 4 threads
    httpd = WsgiBoostHttp(threads=4)
    # Serve static files by "static/*" path from "static" subfolder.
    # Also see template.html.
    httpd.add_static_route('^/static', os.path.join(cwd, 'static'))
    # Set Django WSGI application to be served
    httpd.set_app(application)
    httpd.start()
