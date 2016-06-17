#!/usr/bin/env python
"""
CherryPy benchmark application
"""

import cherrypy


def hello_app(environ, start_response):
    content = b'Hello World!'
    response_headers = [('Content-type', 'text/plain'), ('Content-Length', str(len(content)))]
    start_response('200 OK', response_headers)
    return [content]


if __name__ == '__main__':
    cherrypy.tree.graft(hello_app, '/')
    cherrypy.server.unsubscribe()
    server = cherrypy._cpserver.Server()
    server.socket_host = "0.0.0.0"
    server.socket_port = 8000
    server.thread_pool = 10
    server.subscribe()
    cherrypy.engine.start()
    cherrypy.engine.block()
