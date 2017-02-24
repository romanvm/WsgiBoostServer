WsgiBoostServer Benchmark
#########################
(**Updated on 2017-02-21**)

Below are performance benchmark data for WsgiBoostServer compared to
`Waitress`_ (pure Python) WSGI server and a Node.js server-side JavaScript application.
I wanted my benchmark to be realistic so instead of a function-type
"Hello World!" WSGI application I used `Bottle`_ web-framework to create
a simple REST endpoint.

Test System Configuration
=========================

- CPU: Intel(R) Core(TM) i5-4440 CPU @ 3.10GHz
- RAM: 8 GB DDR3
- OS: Microsoft Windows 7 (6.1) Enterprise Edition 64-bit Service Pack 1 (Build 7601)

Tested Servers
==============

- WsgiBoostServer 1.0.0
- Waitress 1.0.2
- Node.js 6.9.4

Both WSGI servers were run on Python 3.6.0 and served a very simple WSGI application based on `Bottle`_ framework
that sent a fixed set of JSON data, imitating a REST API (see ``test_app.py``).
Node.js also served a very similar server-side application (``node_app.js``) based on `Express`_ framework
that sent the same JSON data.

Performance were measured using Apache Benchmark v.2.3 with the following options::

  ab -n 10000 -c 10 127.0.0.1:8000/api

That is, Apache Benchmark was making 10,000 requests with 10 requests in parallel.

Benchmark Results
=================
WsgiBoostServer (1 thread)
--------------------------

With 1 thread WsgiBoostServer works in fully asynchronous mode using
`Boost.Asio stackful coroutines`_, so even with a single thread
it is still very fast.

::

  Server Software:        WsgiBoost
  Server Hostname:        127.0.0.1
  Server Port:            8000

  Document Path:          /api
  Document Length:        973 bytes

  Concurrency Level:      10
  Time taken for tests:   3.293 seconds
  Complete requests:      10000
  Failed requests:        0
  Total transferred:      11280000 bytes
  HTML transferred:       9730000 bytes
  Requests per second:    3036.74 [#/sec] (mean)
  Time per request:       3.293 [ms] (mean)
  Time per request:       0.329 [ms] (mean, across all concurrent requests)
  Transfer rate:          3345.16 [Kbytes/sec] received

  Connection Times (ms)
                min  mean[+/-sd] median   max
  Connect:        0    0   0.3      0       1
  Processing:     2    3   0.7      3      17
  Waiting:        1    3   0.7      3      16
  Total:          2    3   0.7      3      17

  Percentage of the requests served within a certain time (ms)
    50%      3
    66%      3
    75%      3
    80%      4
    90%      4
    95%      4
    98%      5
    99%      5
   100%     17 (longest request)

WsgiBoostServer (8 threads)
---------------------------

With multiple threads WsgiBoostServer sends data from a WSGI application
in synchronous mode because of Python `Global Interpreter Lock`_ limitations.

::

  Server Software:        WsgiBoost
  Server Hostname:        127.0.0.1
  Server Port:            8000

  Document Path:          /api
  Document Length:        973 bytes

  Concurrency Level:      10
  Time taken for tests:   2.303 seconds
  Complete requests:      10000
  Failed requests:        0
  Total transferred:      11280000 bytes
  HTML transferred:       9730000 bytes
  Requests per second:    4341.73 [#/sec] (mean)
  Time per request:       2.303 [ms] (mean)
  Time per request:       0.230 [ms] (mean, across all concurrent requests)
  Transfer rate:          4782.69 [Kbytes/sec] received

  Connection Times (ms)
                min  mean[+/-sd] median   max
  Connect:        0    0   0.3      0       2
  Processing:     0    2   1.2      2      48
  Waiting:        0    1   1.0      1      36
  Total:          0    2   1.2      2      48

  Percentage of the requests served within a certain time (ms)
    50%      2
    66%      2
    75%      3
    80%      3
    90%      3
    95%      4
    98%      5
    99%      6
   100%     48 (longest request)

Waitress (8 threads)
--------------------

::

  Server Software:        waitress
  Server Hostname:        127.0.0.1
  Server Port:            8000

  Document Path:          /api
  Document Length:        973 bytes

  Concurrency Level:      10
  Time taken for tests:   4.141 seconds
  Complete requests:      10000
  Failed requests:        0
  Total transferred:      11190000 bytes
  HTML transferred:       9730000 bytes
  Requests per second:    2414.63 [#/sec] (mean)
  Time per request:       4.141 [ms] (mean)
  Time per request:       0.414 [ms] (mean, across all concurrent requests)
  Transfer rate:          2638.65 [Kbytes/sec] received

  Connection Times (ms)
                min  mean[+/-sd] median   max
  Connect:        0    0   0.3      0       1
  Processing:     1    4   0.5      4      11
  Waiting:        1    4   0.6      4      11
  Total:          1    4   0.5      4      11

  Percentage of the requests served within a certain time (ms)
    50%      4
    66%      4
    75%      4
    80%      4
    90%      5
    95%      5
    98%      5
    99%      5
   100%     11 (longest request)

Node.js
-------

::

  Server Software:
  Server Hostname:        127.0.0.1
  Server Port:            8000

  Document Path:          /api
  Document Length:        929 bytes

  Concurrency Level:      10
  Time taken for tests:   2.807 seconds
  Complete requests:      10000
  Failed requests:        0
  Total transferred:      11330000 bytes
  HTML transferred:       9290000 bytes
  Requests per second:    3562.17 [#/sec] (mean)
  Time per request:       2.807 [ms] (mean)
  Time per request:       0.281 [ms] (mean, across all concurrent requests)
  Transfer rate:          3941.34 [Kbytes/sec] received

  Connection Times (ms)
  			  min  mean[+/-sd] median   max
  Connect:        0    0   0.3      0       1
  Processing:     1    3   1.3      2      16
  Waiting:        1    3   1.3      2      16
  Total:          1    3   1.3      3      16

  Percentage of the requests served within a certain time (ms)
    50%      3
    66%      3
    75%      3
    80%      3
    90%      4
    95%      4
    98%      7
    99%      8
   100%     16 (longest request)

Conclusion
==========

If we look at "Requests per second" values from the preceding data,
we can see that WsgiBoostServer in multi-threaded mode
has more than 2 times better performance than Waitress
which is one of the fastest pure-Python WSGI servers.
Also it is about 20% faster than a Note.js server serving a similar
REST application.

In a single-threaded mode WsgiBoostServer is still very fast
because of using Boost.Asio asynchronous facilities,
with performance values close to that of Node.js.

However, with "heavier" WSGI frameworks, like Flask or Django, performance
may be significantly lower even with the same JSON data,
but WsgiBoostServer is still faster than pure-Python Waitress.

Although I did not include those data here, it is also worth to note
that with persistent connections (``-k`` option of Apache Benchmark utility)
all servers show about 1.5-2.5 times better performance than without
connection persistence.

All applications used in benchmarks can be found in ``benchmarks`` folder.

.. _Waitress: https://github.com/Pylons/waitress
.. _Bottle: https://bottlepy.org
.. _Express: http://expressjs.com
.. _Boost.Asio stackful coroutines: http://www.boost.org/doc/libs/1_63_0/doc/html/boost_asio.html#boost_asio.overview.core.spawn
.. _Global Interpreter Lock: https://wiki.python.org/moin/GlobalInterpreterLock
