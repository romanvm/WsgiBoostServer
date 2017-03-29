WsgiBoostServer
###############

.. image:: https://travis-ci.org/romanvm/WsgiBoostServer.svg?branch=master
    :target: https://travis-ci.org/romanvm/WsgiBoostServer
.. image:: https://ci.appveyor.com/api/projects/status/5q3hlfplc9xqtm4y/branch/master?svg=true
    :target: https://ci.appveyor.com/project/romanvm/wsgiboostserver

WsgiBoostServer is an asynchronous multi-threaded WSGI and HTTP server written
as a Python extension module in C++ using `Boost.Asio`_ and `Boost.Python`_ libraries.
It can serve both Python WSGI applications and static files.
Because it is written in C++, WsgiBoostServer is faster than pure Python
solutions, like `Waitress`_. It can be used for hosting Python micro-services
and/or static files.

Main Features
=============

**WSGI Server**:

- Compliant with `PEP-3333`_.
- Releases GIL for pure C++ operations, allowing more effective multi-threading.
- Uses ``Transfer-Encoding: chunked`` if a WSGI application does not provide
  ``Content-Length`` header (like Django by default).
- Can be used as a regular module in any Python application.

**HTTP Server**:

- Works on pure C++ level, effectively bypassing GIL.
- Determines MIME for most file types.
- Uses gzip compression for common textual formats: ``txt``/``html``/``xml``/``css``/``js``/``json``.
- Supports ``If-Modified-Since`` and ``If-None-Match`` headers.
- Supports ``Range`` header.
- Checks if requested files are really located within specified content folders
  to forbid requesting files by arbitrary paths (for security reasons).

Benchmarks
==========

Performance benchmarks of WsgiBoostServer compared to pure-Python
`Waitress`_ WSGI server and Node.js can be found `here`_.
According to those benchmarks WsgiBoostServer is about 2 times faster than
Waitress and about 20% faster (with multiple threads) than Node.js.

Compatibility
=============

- **OS**: Windows, Linux. In theory, WsgiBoostServer can be built and used on any OS that has
  a C++ 11/14-compatible compiler and supports Python ``setuptools``.
- **Python**: 2.7 and above (tested with 2.7, 3.5 and 3.6).
- **Boost**: tested with 1.55 and above.
- **Compilers**: GCC 4.9+, MS Visual Studio 2015 Update 2 and above (regardless of Python version).

Usage
=====

Simple example with `Flask`_ micro-framework:

.. code-block:: python

  #!/usr/bin/env python

  from flask import Flask
  from wsgi_boost import WsgiBoostHttp

  app = Flask(__name__)


  # Simple Flask application
  @app.route('/')
  def hello_world():
      return 'Hello World!'


  # Create a server on the port 8080 with 4 threads
  httpd = WsgiBoostHttp(port=8080, threads=4)
  # Set the WSGI app to serve
  httpd.set_app(app)
  # Serve static files from a directory
  httpd.add_static_route('^/files', '/var/www/files')
  httpd.start()

More advanced examples with Flask and Django frameworks can be found ``examples`` folder.

Compilation
===========

See `compilation instructions <Compilation.rst>`_.

.. _Boost.Asio: http://www.boost.org/doc/libs/1_61_0/doc/html/boost_asio.html
.. _Boost.Python: http://www.boost.org/doc/libs/1_61_0/libs/python/doc/html/index.html
.. _Waitress: https://github.com/Pylons/waitress
.. _Flask: http://flask.pocoo.org
.. _PEP-3333: https://www.python.org/dev/peps/pep-3333
.. _here: https://github.com/romanvm/WsgiBoostServer/blob/master/benchmarks/benchmarks.rst
