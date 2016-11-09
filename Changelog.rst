Changelog
=========

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

- Fixed the bug with not sending keep-alive response header when such header is present in http 1.0 request

0.9.1
-----

- Fixed possible race condition in networks with high latencies.

0.9.0
-----

- First public release.
