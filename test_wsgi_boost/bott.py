import os
import sys
import threading
import time
from wsgiref.simple_server import WSGIServer, WSGIRequestHandler
from SocketServer import ThreadingMixIn
from bottle import route, run, template, default_app
from waitress import serve


TEMPLATE = '''
<!DOCTYPE html>
<html>
<head>
<title>Test page</title>
</head>
<body>
<b>Hello {{name}}</b>!
</body>
</html>
'''


class ThreadedWSGIServer(ThreadingMixIn, WSGIServer):
    """Multi-threaded WSGI server"""
    allow_reuse_address = True
    daemon_threads = True


def create_server(app, host='', port=8668):
    """
    Create a new WSGI server listening on 'host' and 'port' for WSGI app
    """
    httpd = ThreadedWSGIServer((host, port), WSGIRequestHandler)
    httpd.set_app(app)
    return httpd


@route('/')
def index():
    return template(TEMPLATE, name='World')


app = default_app()

serve(app, host='0.0.0.0', port=8000)


# httpd = create_server(app, port=8000)
# httpd.serve_forever()

# cwd = os.path.dirname(os.path.abspath(__file__))
# wsgi_boost_dir = os.path.join(os.path.dirname(cwd), 'wsgi_boost')
# sys.path.insert(0, wsgi_boost_dir)

# import wsgi_boost

# print('Startig WsgiBoost server...')
# httpd =  wsgi_boost.WsgiBoostHttp(8000, 8)
# httpd.set_app(app)

# server_thread = threading.Thread(target=httpd.start)
# server_thread.daemon = True
# server_thread.start()
# time.sleep(0.5)

# print('WsgiBoost server started. To stop it press Ctrl+C.')
# try:
#     while True:
#         time.sleep(0.1)
# except KeyboardInterrupt:
#     pass
# finally:
#     httpd.stop()
#     server_thread.join()
# print('WsgiBoost server stopped.')
