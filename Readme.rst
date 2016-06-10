WsgiBoostServer
###############

.. image:: https://travis-ci.org/romanvm/WsgiBoostServer.svg?branch=master
    :target: https://travis-ci.org/romanvm/WsgiBoostServer
.. image:: https://ci.appveyor.com/api/projects/status/5q3hlfplc9xqtm4y/branch/master?svg=true
    :target: https://ci.appveyor.com/project/romanvm/wsgiboostserver

WsgiBoostServer is a multi-threaded WSGI and HTTP server written as a Python extension module
in C++ using `Boost.Asio`_ and `Boost.Python`_. It can serve both Python WSGI applications
and static files. Because it is written in C++, WsgiBoostServer is faster than pure Python
solutions, like `Waitress`_. It can be used for hosting Python micro-services
and/or static files.

Main Features
=============

**WSGI Server**:

- Compliant with `PEP-3333`_.
- Releases GIL for pure C++ operations, allowing more effective multi-threading.
- Can be used as a regular module in any Python application.

**HTTP Server**:

- Works on C++ level, effectively bypassing GIL.
- Determines MIME for most file types.
- Uses gzip compression for common textual formats: ``txt``/``html``/``xml``/``css``/``js``/``json``.
- Supports ``If-Modified-Since`` header.
- Checks if requested files are really located within specified content folders
  to forbid requesting files by arbitrary paths (for security reasons).

Compatibility
=============

- **OS**: Windows, Linux. In theory, WsgiBoostServer can be built and used on any OS that has
  a C++ 11/14-compatible compiler and supports Python ``setuptools``.
- **Python**: 2.7+ (tested on 2.7 and 3.5).
- **Boost**: tested with 1.58, 1.60 and 1.61, but probably will work with earlier versions
  that are not too old.
- **Compilers**: MS Visual Studio 2015 Update 2+ (regardless of Python version), GCC 5.0+.

Usage
=====

Simple example using `Flask`_ micro-framework::

    #!/usr/bin/env python

    from flask import Flask
    import wsgi_boost

    app = Flask(__name__)


    # Simple Flask application
    @app.route('/')
    def hello_world():
        return 'Hello World!'


    # Create a server on the default port 8000 with 4 threads
    httpd = wsgi_boost.WsgiBoostHttp(num_threads=4)
    httpd.set_app(app)
    httpd.start()

Also see ``examples`` folder.

Compilation
===========

Normally, WsgiBoostServer is compiled using ``setuptools`` setup script.

Linux
-----

WsgiBoostServer can be compiled on any Linux distributive that uses GCC 5.0+ as the default system compiler,
for example Ubuntu 16.04 LTS (Xenial Xerus). The procedure below assumes that you are using Ubuntu 16.04.

First, install prerequisites::

  $ sudo apt-get install build-essential python-dev python-pip zlib1g-dev libbz2-dev libboost-all-dev

To build against Python 3 you need to install the respective libraries, for example ``python3-dev``
and ``python3-pip``.

Then build::

  $ python setup.py build

To install in the current Python environment::

  $ python setup.py install

Run tests::

  $ python setup.py test

To build against Python 3 use ``python3`` instead of ``python``
(on those distributions that still use Python 2 by default).

Windows
-------

TBD (see ``appveyour.yml``)

.. _Boost.Asio: http://www.boost.org/doc/libs/1_61_0/doc/html/boost_asio.html
.. _Boost.Python: http://www.boost.org/doc/libs/1_61_0/libs/python/doc/html/index.html
.. _Waitress: https://github.com/Pylons/waitress
.. _Flask: http://flask.pocoo.org
.. _PEP-3333: https://www.python.org/dev/peps/pep-3333
