WsgiBoostServer
===============

.. image:: https://travis-ci.org/romanvm/WsgiBoostServer.svg?branch=master
    :target: https://travis-ci.org/romanvm/WsgiBoostServer

WsgiBoostServer is a multi-threaded WSGI and HTTP server written as a Python extension module
in C++ using `Boost.Asio`_ and `Boost.Python`_. It can serve both Python WSGI applications
and static files.

Main Features
-------------

**WSGI Server**

- PEP-3333-compliant
- Releases GIL for pure C++ operations allowing more effective multi-threading
- Can be used as a regular module in any Python application

**HTTP Server**

- Works on C++ level effectively bypassing GIL
- Determines MIME for most file types.
- Uses gzip compression for common textual formats: txt/html/xml/css/js/json
- Supports ``If-Modified-Since`` header

.. _Boost.Asio: http://www.boost.org/doc/libs/1_61_0/doc/html/boost_asio.html
.. _Boost.Python: http://www.boost.org/doc/libs/1_61_0/libs/python/doc/html/index.html
