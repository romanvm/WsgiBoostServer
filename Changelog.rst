Changelog
=========

2.0.0
-----

- Converted Python bindings from Boost.Python to Pybind11.
- Dropped Python 2 support.
- Implemented HTTPS support (with full code refactoring).

1.0.4
-----

- Minor updates.

1.0.3
-----

- Fixed potential Python crash when sending a HTML error message.

1.0.2
-----

- Implemented the pool of ``boost::asio::io_service`` that allows to send WSGI application data
  in fully asynchronous mode.
- Various internal optimizations.

1.0.1a
------

- Fixed missing library dependency in ``setup.py`` build script.
- The ``setup.py`` build script now takes ``--boost-headers`` and ``--boost-libs`` options
  that allow to explicitly set locations of Boost headers and pre-compiled libraries.
- No changes to WsgiBoostServer code.

1.0.1
-----

- Fixed ``boost::asio::strand`` usage.
- Fixed ``Content-Length`` header in ``206 Partial Content``
  responses for static content.
- Fixed ``wsgi.input`` in Python 3 so that now it correctly produces bytes
  instead of strings.
- Now in multi-threaded mode WsgiBoostServer uses asynchronous operations
  for most common WSGI application scenarios.

1.0.0
-----

- Renamed ``num_threads`` constructor parameter to ``threads`` (**breaking change!**).
- Now ``Transfer-Encoding: chunked`` is used if a WSGI application
  does not provide ``Content-Length`` header.
- Error messages in HTML.
- Code optimization and bugfixes.

0.9.8
-----

- Now the WSGI server is fully asynchronous in the single-thread mode.
- Added ``static_cache_control`` property for setting the value of ``Cache-Control`` HTTP header
  for static content.

0.9.7
-----

- Now asynchronous network operations are used outside Python GIL,
  e.g. for sending static content.

0.9.6
-----

- Fixed handling requests with Range headers.

0.9.5
-----

- Fixed a bug when ``Connection::read_bytes`` method did not decrease ``m_bytes_left`` counter.
- Buffer the first 4KB of data sent in ``POST``/``PUT`` request to a WSGI application.

0.9.4
-----

- Changed time string parsing method to support GCC 4.9 compiler on Linux.
- Added support for "Range" request headers when getting static content.

0.9.3
-----

- Added "Connection: close" header for non-persistent requests.

0.9.2
-----

- Fixed the bug with not sending keep-alive response header when such header is present
  in http 1.0 request

0.9.1
-----

- Fixed possible race condition in networks with high latencies.

0.9.0
-----

- First public release.
