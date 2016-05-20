#!/usr/bin/env python
# coding: utf-8

from __future__ import print_function
import os
import sys
import threading
import time
import unittest
from contextlib import closing
import requests

print('Running Python tests')

cwd = os.path.dirname(os.path.abspath(__file__))
os.chdir(cwd)
project_dir = os.path.dirname(cwd)
sys.path.insert(0, project_dir)

import wsgi_boost


class ServingStaticFilesTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._httpd = wsgi_boost.WsgiBoostHttp(8000, 4)
        cls._server_thread = threading.Thread(target=cls._httpd.start)
        cls._httpd.add_static_route('^/static', cwd)
        cls._httpd.add_static_route('^/invalid_dir', '/foo/bar/baz/')
        cls._server_thread.start()
        time.sleep(0.5)

    @classmethod
    def tearDownClass(cls):
        cls._httpd.stop()
        del cls._httpd

    def test_forbidden_http_methods(self):
        resp = requests.post('http://127.0.0.1:8000/static')
        self.assertEqual(resp.status_code, 405)

    def test_invalid_content_directory(self):
        resp = requests.get('http://127.0.0.1:8000/invalid_dir')
        self.assertEqual(resp.status_code, 500)
        self.assertTrue('Invalid content directory' in resp.text)

    def test_accessing_static_file(self):
        resp = requests.get('http://127.0.0.1:8000/static/index.html')
        self.assertEqual(resp.status_code, 200)
        self.assertTrue('Welcome to Our Company' in resp.text)
        resp = requests.get('http://127.0.0.1:8000/static/foo/bar.html')
        self.assertEqual(resp.status_code, 404)

    def test_gzipping_static_files(self):
        html_size = os.path.getsize('index.html')
        resp = requests.get('http://127.0.0.1:8000/static/index.html')
        self.assertTrue(html_size > int(resp.headers['Content-Length']))
        png_size = os.path.getsize('profile_pic.png')
        resp = requests.get('http://127.0.0.1:8000/static/profile_pic.png')
        self.assertEqual(png_size, int(resp.headers['Content-Length']))


if __name__ == '__main__':
    unittest.main()
