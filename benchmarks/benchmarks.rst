WsgiBoostServer Benchmark
#########################

Below are performance benchmark data for WsgiBoostServer compared to popular
Python WSGI servers.


Test System Configuration
=========================

- CPU: Intel Pentium E5400, 2.7 GHz
- RAM: 4 GB DDR2
- OS: MS Windows 10 (1511)

Tested WSGI Servers
===================

- WsgiBoostServer 0.9.1
- `Waitress`_ 0.9.0
- `CherryPy`_ 6.0.1
- `Twisted`_ 16.2.0

All servers were run from Python 2.7.11 and served a simple WSGI application:

.. code-block:: python

  def hello_app(environ, start_response):
      content = b'Hello World!'
      response_headers = [('Content-type', 'text/plain'), ('Content-Length', str(len(content)))]
      start_response('200 OK', response_headers)
      return [content]

Performance were measured using
Apache Benchmark v.2.3 with the following options::

  ab -n 10000 -c 10 127.0.0.1:8000/

That is, Apache Benchmark was making 10,000 requests with 10 requests in parallel.

Benchmark Results
=================

WsgiBoostServer (10 threads)
----------------------------

::

  Server Software:        WsgiBoost
  Server Hostname:        127.0.0.1
  Server Port:            8000

  Document Path:          /
  Document Length:        12 bytes

  Concurrency Level:      10
  Time taken for tests:   4.815 seconds
  Complete requests:      10000
  Failed requests:        0
  Total transferred:      1480000 bytes
  HTML transferred:       120000 bytes
  Requests per second:    2076.79 [#/sec] (mean)
  Time per request:       4.815 [ms] (mean)
  Time per request:       0.482 [ms] (mean, across all concurrent requests)
  Transfer rate:          300.16 [Kbytes/sec] received

  Connection Times (ms)
                min  mean[+/-sd] median   max
  Connect:        0    0   0.3      0       3
  Processing:     0    4   5.8      4     401
  Waiting:        0    3   5.6      3     395
  Total:          1    5   5.8      5     402

  Percentage of the requests served within a certain time (ms)
    50%      5
    66%      5
    75%      5
    80%      5
    90%      6
    95%      7
    98%     11
    99%     19
   100%    402 (longest request)

Waitress
--------

::

  Server Software:        waitress
  Server Hostname:        127.0.0.1
  Server Port:            8000

  Document Path:          /
  Document Length:        12 bytes

  Concurrency Level:      10
  Time taken for tests:   10.525 seconds
  Complete requests:      10000
  Failed requests:        0
  Total transferred:      1510000 bytes
  HTML transferred:       120000 bytes
  Requests per second:    950.16 [#/sec] (mean)
  Time per request:       10.525 [ms] (mean)
  Time per request:       1.052 [ms] (mean, across all concurrent requests)
  Transfer rate:          140.11 [Kbytes/sec] received

  Connection Times (ms)
                min  mean[+/-sd] median   max
  Connect:        0    0   0.4      0       6
  Processing:     4   10   6.5      9      71
  Waiting:        0   10   6.5      9      71
  Total:          4   10   6.5      9      71

  Percentage of the requests served within a certain time (ms)
    50%      9
    66%     10
    75%     10
    80%     11
    90%     13
    95%     18
    98%     38
    99%     43
   100%     71 (longest request)

CherryPy (10 threads)
---------------------

::

  Server Software:        0.0.0.0
  Server Hostname:        127.0.0.1
  Server Port:            8000

  Document Path:          /
  Document Length:        12 bytes

  Concurrency Level:      10
  Time taken for tests:   107.959 seconds
  Complete requests:      10000
  Failed requests:        0
  Total transferred:      1330000 bytes
  HTML transferred:       120000 bytes
  Requests per second:    92.63 [#/sec] (mean)
  Time per request:       107.959 [ms] (mean)
  Time per request:       10.796 [ms] (mean, across all concurrent requests)
  Transfer rate:          12.03 [Kbytes/sec] received

  Connection Times (ms)
                min  mean[+/-sd] median   max
  Connect:        0    9  66.7      0     588
  Processing:     3   98 182.8     18    1006
  Waiting:        3   96 180.6     17     600
  Total:          4  108 190.5     18    1007

  Percentage of the requests served within a certain time (ms)
    50%     18
    66%     23
    75%     29
    80%     36
    90%    509
    95%    513
    98%    525
    99%    530
   100%   1007 (longest request)

Twisted
-------

The ``Twisted`` server was launched with the following command::

  python twistd.py -n --logfile=NUL web --port 8000 --wsgi wsgi_boost_bench.hello_app

The ``--logfile=NUL`` option was used to disable logging requests in the console,
because writing into the console can seriously degrade server performance.

Benchmark results::

  Server Software:        TwistedWeb/16.2.0
  Server Hostname:        127.0.0.1
  Server Port:            8000

  Document Path:          /
  Document Length:        12 bytes

  Concurrency Level:      10
  Time taken for tests:   27.327 seconds
  Complete requests:      10000
  Failed requests:        0
  Total transferred:      1410000 bytes
  HTML transferred:       120000 bytes
  Requests per second:    365.94 [#/sec] (mean)
  Time per request:       27.327 [ms] (mean)
  Time per request:       2.733 [ms] (mean, across all concurrent requests)
  Transfer rate:          50.39 [Kbytes/sec] received

  Connection Times (ms)
                min  mean[+/-sd] median   max
  Connect:        0    0   0.5      0       4
  Processing:     6   27  10.6     23      85
  Waiting:        5   24  10.2     20      83
  Total:          6   27  10.6     23      86

  Percentage of the requests served within a certain time (ms)
    50%     23
    66%     26
    75%     29
    80%     31
    90%     41
    95%     51
    98%     61
    99%     69
   100%     86 (longest request)

Conclusion
==========

As you can see from the preceding data, WsgiBoostServer has more than
2 times better performance than Waitress which is the fastest
Python-based WSGI server here.

Although I did not include those data here, it is also worth to note
that with persistent connections (with ``Connection: keep-alive`` header)
both WsgiBoostServer and Waitress show about 2 times better performance
than without re-using connections.

All applications used in benchmarks can be found in ``benchmarks`` folder.

.. _Waitress: https://github.com/Pylons/waitress
.. _CherryPy: http://www.cherrypy.org
.. _Twisted: https://twistedmatrix.com/trac
