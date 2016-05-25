#!/usr/bin/env python
# coding: utf-8

from __future__ import print_function
import os
import sys
import threading
import time
import unittest
import requests
from wsgiref.validate import validator

print('Running Python tests')

cwd = os.path.dirname(os.path.abspath(__file__))
os.chdir(cwd)
sys.path.insert(0, os.path.dirname(cwd))

import wsgi_boost


class ServingStaticFilesTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._httpd = wsgi_boost.WsgiBoostHttp(num_threads=1)
        cls._httpd.add_static_route('^/static', cwd)
        cls._httpd.add_static_route('^/invalid_dir', '/foo/bar/baz/')
        cls._server_thread = threading.Thread(target=cls._httpd.start)
        cls._server_thread.daemon = True
        cls._server_thread.start()

    @classmethod
    def tearDownClass(cls):
        cls._httpd.stop()
        cls._server_thread.join()
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

    def test_if_modified_since(self):
        last_modified = time.mktime(time.strptime('2014-12-21 17:20:00', '%Y-%m-%d %H:%M:%S'))
        os.utime(os.path.join(cwd, 'index.html'), (last_modified, last_modified))
        resp = requests.get('http://127.0.0.1:8000/static/index.html', headers={'If-Modified-Since': 'Sat, 20 Dec 2014 15:00:00 GMT'})
        self.assertEqual(resp.status_code, 200)
        resp = requests.get('http://127.0.0.1:8000/static/index.html', headers={'If-Modified-Since': 'Mon, 22 Dec 2014 15:00:00 GMT'})
        self.assertEqual(resp.status_code, 304)


class App(object):
    """
    Test WSGI application
    """
    def __call__(self, environ, start_response):
        self.environ = environ
        self.start_response = start_response
        content = 'App OK'
        if self.environ['PATH_INFO'] == '/test_write':
            content = 'Write OK'
        elif self.environ['PATH_INFO'] == '/test_input_read':
            self.test_input_read()
            content = 'Input read OK'
        write = start_response('200 OK', [('Content-type', 'text/plain'), ('Content-Length', str(len(content)))])
        if content == 'Write OK':
            write(content)
            content = ''
        return [content]

    def test_input_read(self):
        content = self.environ['wsgi.input'].read()
        assert len(content) == 4194


class WsgiServerTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._httpd = wsgi_boost.WsgiBoostHttp(num_threads=1)
        app = App()
        cls._httpd.set_app(validator(app))
        cls._server_thread = threading.Thread(target=cls._httpd.start)
        cls._server_thread.daemon = True
        cls._server_thread.start()
        with open('german.txt', mode='r') as fo:
            cls._data = fo.read()

    @classmethod
    def tearDownClass(cls):
        cls._httpd.stop()
        cls._server_thread.join()
        del cls._httpd

    def test_wsgi_app_response(self):
        resp = requests.get('http://127.0.0.1:8000/')
        self.assertEqual(resp.status_code, 200)
        self.assertTrue('App OK' in resp.text)

    def test_write_function(self):
        resp = requests.get('http://127.0.0.1:8000/test_write')
        self.assertEqual(resp.status_code, 200)
        self.assertTrue('Write OK' in resp.text)

    def test_input_read(self):
        resp = requests.post('http://127.0.0.1:8000/test_input_read', data=self._data)
        self.assertEqual(resp.status_code, 200)
        self.assertTrue('Input read OK' in resp.text)


if __name__ == '__main__':
    unittest.main()
