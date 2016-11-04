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

- Works on pure C++ level, effectively bypassing GIL.
- Determines MIME for most file types.
- Uses gzip compression for common textual formats: ``txt``/``html``/``xml``/``css``/``js``/``json``.
- Supports ``If-Modified-Since`` header.
- Supports ``Range`` header.
- Checks if requested files are really located within specified content folders
  to forbid requesting files by arbitrary paths (for security reasons).

Benchmarks
==========

Performance benchmarks of WsgiBoostServer compared to popular Python WSGI servers
can be found `here`_. According to those benchmarks WsgiBoostServer has about
2 times better performance than the best Python-based WSGI server.

Compatibility
=============

- **OS**: Windows, Linux. In theory, WsgiBoostServer can be built and used on any OS that has
  a C++ 11/14-compatible compiler and supports Python ``setuptools``.
- **Python**: 2.7+ (tested on 2.7 and 3.5).
- **Boost**: tested with 1.58, 1.60-1.62, but probably will work with earlier versions
  that are not too old.
- **Compilers**: GCC 4.9+, MS Visual Studio 2015 Update 2+ (regardless of Python version).

Usage
=====

Simple example using `Flask`_ micro-framework:

.. code-block:: python

    #!/usr/bin/env python

    from flask import Flask
    import wsgi_boost

    app = Flask(__name__)


    # Simple Flask application
    @app.route('/')
    def hello_world():
        return 'Hello World!'


    # Create a server on the port 8080 with 4 threads
    httpd = wsgi_boost.WsgiBoostHttp(port=8080, num_threads=4)
    httpd.set_app(app)
    httpd.start()

Also see ``examples`` folder.

Compilation
===========

Normally, WsgiBoostServer is compiled using ``setuptools`` setup script.

Linux
-----

WsgiBoostServer can be compiled on any Linux distributive that uses GCC 4.9+ as the default system compiler,
for example Ubuntu 16.04 LTS (Xenial Xerus). The procedure below assumes that you are using Ubuntu 16.04.

First, install prerequisites::

  $ sudo apt-get install build-essential python-dev python-pip zlib1g-dev libbz2-dev libboost-all-dev

Then download or clone WsgiBoostServer sources to your computer.

To build against Python 3 you need to install the respective libraries, for example ``python3-dev``
and ``python3-pip``.

Then build::

  $ python setup.py build

To install into the current Python environment::

  $ python setup.py install

Run tests::

  $ python setup.py test

To build against Python 3 use ``python3`` instead of ``python``
(on those distributions that still use Python 2 by default).

Alternatively, you can install WsgiBoostServer using ``pip``::

  $ pip install git+https://github.com/romanvm/WsgiBoostServer.git#egg=WsgiBoostServer

``pip`` will download WsgiBoostServer sources, compile the binary module
and install it into your working Python environment.

**Note**: On **Releases** tab of this repository you can find a compiled wheel
for Python 2.7 on Raspberry Pi 2.

Windows
-------

Compiled wheels for Python 2.7 and 3.5 (32 bit) can be downloaded from "**Releases**" tab of this repository.
If you want to compile WsgiBoostServer for Windows yourself, follow the instruction below.
You can also check AppVeyor CI build configuration ``appveyor.yml``.

**Tools required**: MS Visual Studio 2015 Update 2+, Cmake

Note that WsgiBoostServer ``setup.py`` script monkey-patches the default ``distutils`` complier on Windows
and uses MS Visual Studio 2015 regardless of Python version used to compile the extension module.

Procedure
~~~~~~~~~

Download ``zlib`` sources from `zlib Home Site`_ and unpack them into the folder of your choice,
for example ``c:\zlib``.

Open console, go to the ``zlib`` folder and execute there::

  >cmake .

You don't need to compile ``zlib``, ``Boost.Buld`` will do that for you.

Now download ``boost`` sources from `Boost Home Site`_  and unpack them into the folder of your choice,
for example ``c:\boost``.

Open Windows console, go to the ``boost`` folder and execute there::

  >bootstrap

After the bootstrap script finishes building Boost.Build engine, create Boost.Build configuration file
``user-config.jam`` in your ``%USERPROFILE%`` folder with the following content::

  using python : 3.5 : c:\\Python35-32 ;
  using msvc : 14.0 ;

The ``using python`` parameter should point to the Python version that will be used for building
WsgiBoostServer. Change it if necessary.

Now open the console, go to the ``boost`` folder and execute there::

  >b2 link=static runtime-link=static variant=release --stagedir=c:\boost\msvc14x32 -sZLIB_SOURCE=c:\zlib --with-regex --with-system --with-coroutine --with-context --with-filesystem --with-iostreams --with-date_time --with-python

Note that ``-sZLIB_SOURCE`` option should point to your actual ``zlib`` folder.

Boost.Build engine will build the necessary libraries to link WsgiBoostServer against and place them into
``c:\boost\msvc14x32\lib`` folder. This folder is set by the ``--stagedir`` option.

Now you need to set the necessary environment variables. Execute the following commands::

  >setx BOOST_ROOT c:\boost
  >setx BOOST_LIBRARYDIR c:\boost\msvc14x32\lib

The variables should point to actual folders where Boost header files and libraries are located. Now restart your computer
or sign out and then sign in again.

Now you can build and install WsgiBoostServer using the ``setup.py`` script or Python ``pip``
as described in the preceding `Linux`_ section. Note that you must use the same Python version that was used to build
Boost.Python library.

.. _Boost.Asio: http://www.boost.org/doc/libs/1_61_0/doc/html/boost_asio.html
.. _Boost.Python: http://www.boost.org/doc/libs/1_61_0/libs/python/doc/html/index.html
.. _Waitress: https://github.com/Pylons/waitress
.. _Flask: http://flask.pocoo.org
.. _PEP-3333: https://www.python.org/dev/peps/pep-3333
.. _here: https://github.com/romanvm/WsgiBoostServer/blob/master/benchmarks/benchmarks.rst
.. _zlib Home Site: http://www.zlib.net
.. _Boost Home Site: http://www.boost.org
