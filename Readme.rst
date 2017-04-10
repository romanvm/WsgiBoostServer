WsgiBoostServer
###############

.. image:: https://travis-ci.org/romanvm/WsgiBoostServer.svg?branch=master
    :target: https://travis-ci.org/romanvm/WsgiBoostServer
.. image:: https://ci.appveyor.com/api/projects/status/5q3hlfplc9xqtm4y/branch/master?svg=true
    :target: https://ci.appveyor.com/project/romanvm/wsgiboostserver

WsgiBoostServer is an asynchronous multi-threaded WSGI and HTTP server written
as a Python extension module in C++ using `Boost.Asio`_ and `Pybind11`_ libraries.
It can serve both Python WSGI applications and static files.
Because it is written in C++, WsgiBoostServer is faster than pure Python
solutions, like `Waitress`_. It can be used for hosting Python micro-services
and/or static files. WsgiBoostServer also supports HTTPS protocol.

In **Releases** section of this repository you can find compiled wheels with HTTPS support
(statically linked against Boost libraries) for Python 3.6 on Windows (32 and 64 bit),
for various Python 3 versions on Linux x64, and for Python 3.4 on Raspberry Pi 2 (if I'm not too lazy to compile this).

You can install those binary wheels into your Python environment with pip::

  $pip install <link to a .whl file>


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
- **Python 3** (tested with 3.4, 3.5 and 3.6). Python 2 is no longer supported!
- **Boost**: tested with 1.55 and above.
- **Compilers**: GCC 4.9+, MS Visual Studio 2015 Update 3 and above.

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
Also see docstrings in ``wsgi_boost/wsgi_boost.cpp`` file for detailed information on
WsgiBoostServer API.

HTTPS Support
=============

WsgiBoostServer provides ``WsgiBoostHttps`` class that allows to serve your
WSGI application and static files via a secure connection. To use HTTPS,
in the preceding example you need to replace ``WsgiBoostHttp`` class
with ``WsgiBoostHttps`` like this:

.. code-block:: python

  httpd = WsgiBoostHttps('server.crt', 'server.key', port=443, threads=4)
  # Redirect all non-secure HTTP requests from port 80 (default) to HTTPS port
  httpd.redirect_http = True

Here ``'server.crt'`` and ``'server.key'`` are paths to HTTPS certificate
and private key files respectively. If your private key is password-protected
you can provide a password via ``HTTPS_KEY_PASS`` environment variable.

It is recommended to obtain a HTTPS certificate from a trusted Certificate Authority
but for testing purposes you can create a `self-signed certificate`_.
Note that most programs won't accept such certificate with default security
settings. For example, in browsers you need to add your site to browser's security
exceptions. With ``curl`` you need to use ``-k`` option, and with Python ``requests``
library you need to provide ``verify=False`` argument to request functions.

Optionally, you can generate parameters for `Diffie-Hellman`_ key exchange::

  $openssl dhparam -out dh.pem 2048

It is strongly recommended to use at least 2048 bit prime number length.
A path to the generated file can be passed as the third positional parameter
to ``WsgiBoostHttps`` constructor.

If you you want to get a free trusted certificate from `Let's Enccrypt`_ for a site
hosted on WsgiBoostServer, you can use their ``certbot`` utility with ``--webroot`` option.
In this case before obtaining a certificate you need to add a static route
to ``--webroot-path`` folder::

  httpd.add_static_route(r'^/\.well-known', '/path/to/webroot-dir')


Compilation
===========

See `compilation instructions <Compilation.rst>`_.

.. _Boost.Asio: http://www.boost.org/doc/libs/1_61_0/doc/html/boost_asio.html
.. _Pybind11: https://github.com/pybind/pybind11
.. _Waitress: https://github.com/Pylons/waitress
.. _Flask: http://flask.pocoo.org
.. _PEP-3333: https://www.python.org/dev/peps/pep-3333
.. _here: https://github.com/romanvm/WsgiBoostServer/blob/master/benchmarks/benchmarks.rst
.. _self-signed certificate: http://www.akadia.com/services/ssh_test_certificate.html
.. _Diffie-Hellman: https://en.wikipedia.org/wiki/Diffie%E2%80%93Hellman_key_exchange
.. _Let's Enccrypt: https://letsencrypt.org
