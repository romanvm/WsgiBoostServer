#!/usr/bin/env python
"""
This example demonstrates serving a Flask application
that returns a rendered web-page with static content (a picture)
"""

import os
import sys
from flask import Flask, render_template, make_response

cwd = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(cwd))

import wsgi_boost

app = Flask(__name__)


# Simple Flask application
@app.route('/')
@app.route('/<name>')
def hello_world(name='Roman'):
    resp = make_response(render_template('template.html', name=name))
    resp.headers['Content-Type'] = 'text/html'
    return resp


# Create a server on the default port 8000 with 4 threads
httpd = wsgi_boost.WsgiBoostHttp(num_threads=4)
httpd.add_static_route('^/static', cwd)
httpd.set_app(app)
httpd.start()
