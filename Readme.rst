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

Normally, WsgiBoostServer is compiled using ``setup.py`` script.

Linux
-----

WsgiBoostServer can be compiled on any Linux distributive that uses GCC 4.9+ as the default system compiler,
for example Ubuntu 16.04 LTS (Xenial Xerus). The procedure below assumes that you are using Ubuntu 16.04.

Python and Boost From System Repositories
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following procedure assumes that you are using Python and Boost from system repositories. For example,
Ubuntu 16.04 LTS (Xenial Xerus) provides Python 2.7, 3.5 and Boost 1.58.

- Install prerequisites::

    $ sudo apt-get install build-essential python-dev python-pip zlib1g-dev libbz2-dev libboost-all-dev

  To build against Python 3 you need to install the respective libraries, for example ``python3-dev``
  and ``python3-pip``.

- Download or clone WsgiBoostServer sources to your computer.

- Activate your working Python virtual environment (optional).

- Build WsgiBoostServer::

    $ python setup.py build

  To build against Python 3 use ``python3`` instead of ``python``
  (on those Linux distributions that still use Python 2 by default).

- Run tests (optional)::

    $ python setup.py test

- Install WsgiBoostServer::

    $ python setup.py install

  It is strongly recommended to use Python virtual environments.

Alternatively, after installing prerequisites you can install WsgiBoostServer directly from GitHub repository
using ``pip``::

  $ pip install git+https://github.com/romanvm/WsgiBoostServer.git#egg=WsgiBoostServer

``pip`` will download WsgiBoostServer sources, compile the binary module
and install it into your working Python environment.

Custom Python and Boost Versions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following procedure assumes that you want to use custom Python and/or Boost versions.

- Install prerequisites::

    $ sudo apt-get install build-essential zlib1g-dev libbz2-dev

- Install the necessary Python development package. For example, `deadsnakes`_ ppa repository provides various
  Python versions for Ubuntu. Let's assume we want to build WsgiBoostServer with Python 3.6::

    $ sudo add-apt-repository ppa:fkrull/deadsnakes
    $ sudo apt-get update
    $ sudo apt-get install python3.6 python3.6-dev

- Downloaded the necessary Boost version from `Boost Home Site`_ and unpack it to a folder
  of your choice, for example ``$HOME/boost``.

- Go to Boost folder and run::

    $ sh bootstrap.sh

- After the bootstrap script finishes building Boost.Build engine, create Boost.Build configuration file
  ``user-config.jam`` in your ``$HOME`` folder with the following content::

    using python : 3.6 ;

  The ``using python`` parameter should point to the Python version that will be used for building
  WsgiBoostServer. Change it if necessary. See `Boost.Build documentation`_ for more info about
  how to configure Boost.Build with custom Python versions.

- Go to Boost folder and run there::

    $ ./b2 link=static variant=release cxxflags=-fPIC --layout=system --with-regex --with-system --with-coroutine --with-context --with-filesystem --with-iostreams --with-date_time --with-python

  Boost.Build engine will build the necessary libraries to link WsgiBoostServer against and place them into
  ``$HOME/boost/stage/lib`` folder. This folder can be changed by the ``--stagedir=`` option.

- Build WsgiBoostServer using ``setup.py`` script::

    $ python3.6 setup.py build --boost-headers="$HOME/boost" --boost-libs="$HOME/boost/stage/lib"

  The ``--boost-headers=`` and ``--boost-libs=`` options must point to actual folders where Boost header files and libraries are located.
  Note that you must use the same Python version that was used to build Boost.Python library.

- Install WsgiBoostServer::
  
    $ python3.6 setup.py install

  It is strongly recommended to use Python virtual environments.

**Note**: On **Releases** tab of this repository you can find statically compiled wheels
for Python 2.7 and 3.4 on Raspberry Pi 2.

Windows
-------

Compiled wheels for Python 2.7 and 3.6 (32 bit) can be downloaded from "**Releases**" tab of this repository.
If you want to compile WsgiBoostServer for Windows yourself, follow the instruction below.
You can also check AppVeyor CI build configuration ``appveyor.yml``.

**Tools required**: MS Visual Studio 2015 Update 2+, Cmake

Note that WsgiBoostServer ``setup.py`` script monkey-patches the default ``distutils`` complier on Windows
and uses MS Visual Studio 2015 regardless of Python version used to compile the extension module.

Procedure
~~~~~~~~~

The build procedure is similar to that for custom Python and Boost versions on Linux.

- Download ``zlib`` sources from `zlib Home Site`_ and unpack them into the folder of your choice,
  for example ``c:\zlib``.

- Open console, go to the ``zlib`` folder and execute there::

    >cmake .

  You don't need to compile ``zlib``, ``Boost.Buld`` will do that for you.

- Download ``boost`` sources from `Boost Home Site`_  and unpack them into the folder of your choice,
  for example ``c:\boost``.

- Open Windows console, go to the ``boost`` folder and execute there::

    >bootstrap

- After the bootstrap script finishes building Boost.Build engine, create Boost.Build configuration file
  ``user-config.jam`` in your ``%USERPROFILE%`` folder with the following content::

    using python : 3.6 : c:\\Python36-32 ;
    using msvc : 14.0 ;

  The ``using python`` parameter should point to the Python version that will be used for building
  WsgiBoostServer. Change it if necessary.

- Open the console, go to the ``boost`` folder and execute there::

    >b2 link=static runtime-link=static variant=release -sZLIB_SOURCE=c:\zlib --with-regex --with-system --with-coroutine --with-context --with-filesystem --with-iostreams --with-date_time --with-python

  Note that ``-sZLIB_SOURCE`` option should point to your actual ``zlib`` folder.

  Boost.Build engine will build the necessary libraries to link WsgiBoostServer against and place them into
  ``c:\boost\stage\lib`` folder. This folder can be changed by the ``--stagedir=`` option.

- Build WsgiBoostServer using ``setup.py`` script::

    >python setup.py build --boost-headers="c:\boost" --boost-libs="c:\boost\stage\lib"

  The ``--boost-headers=`` and ``--boost-libs=`` options must point to the actual folders where Boost header files and libraries are located.
  Note that you must use the same Python version that was used to build Boost.Python library.

- Install WsgiBoostServer::

    >python setup.py install

  It is strongly recommended to use Python virtual environments.

.. _Boost.Asio: http://www.boost.org/doc/libs/1_61_0/doc/html/boost_asio.html
.. _Boost.Python: http://www.boost.org/doc/libs/1_61_0/libs/python/doc/html/index.html
.. _Waitress: https://github.com/Pylons/waitress
.. _Flask: http://flask.pocoo.org
.. _PEP-3333: https://www.python.org/dev/peps/pep-3333
.. _here: https://github.com/romanvm/WsgiBoostServer/blob/master/benchmarks/benchmarks.rst
.. _zlib Home Site: http://www.zlib.net
.. _Boost Home Site: http://www.boost.org
.. _deadsnakes: https://launchpad.net/~fkrull/+archive/ubuntu/deadsnakes
.. _Boost.Build documentation: http://www.boost.org/doc/libs/1_63_0/libs/python/doc/html/building/configuring_boost_build.html
