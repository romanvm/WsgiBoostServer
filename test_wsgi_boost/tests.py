#!/usr/bin/env python
# coding: utf-8

from __future__ import print_function
import os
import sys
import threading
import time
import requests
import unittest

print('Running Python tests')

cwd = os.path.dirname(os.path.abspath(__file__))
project_dir = os.path.dirname(cwd)
wsgi_boost_dir = os.path.join(project_dir, 'wsgi_boost')
sys.path.insert(0, wsgi_boost_dir)

import wsgi_boost


class ServingStaticFilesTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._httpd = wsgi_boost.WsgiBoostHttp(8000, 4)
        cls._server_thread = threading.Thread(target=cls._httpd.start)
        cls._server_thread.start()
        time.sleep(0.5)

    @classmethod
    def tearDownClass(cls):
        cls._httpd.stop()
        del cls._httpd

    def test_forbidden_http_methods(self):
        self._httpd.add_static_route('^/static', cwd)
        resp = requests.post('http://127.0.0.1:8000/static')
        self.assertEqual(resp.status_code, 405)

    def test_invalid_content_directory(self):
        self._httpd.add_static_route('^/invalid_dir', '/foo/bar/baz/')
        resp = requests.get('http://127.0.0.1:8000/invalid_dir')
        self.assertEqual(resp.status_code, 500)
        self.assertTrue('Invalid content directory' in resp.text)


if __name__ == '__main__':
    unittest.main()
