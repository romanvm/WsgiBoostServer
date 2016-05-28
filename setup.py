#!/usr/bin/env python

import os
import sys
from distutils.core import setup
from distutils.extension import Extension


class BuildError(Exception):
    pass

extra_compile_args = ['-std=c++11']

if sys.platform == 'win32':
    try:
        boost_root = os.environ['BOOST_ROOT']
    except KeyError:
        raise BuildError('%BOOST_ROOT% variable is not defined!')
    try:
        boost_librarydir = os.environ['BOOST_LIBRARYDIR']
    except KeyError:
        raise BuildError('%BOOST_LIBRARYDIR% is not defined!')
    include_dirs = ['./wsgi_boost', boost_root]
    libraries=[
        'mswsock',
        'ws2_32',
        'boost_regex-mt-s',
        'boost_system-mt-s',
        'boost_thread-mt-s',
        'boost_coroutine-mt-s',
        'boost_context-mt-s',
        'boost_filesystem-mt-s',
        'boost_date_time-mt-s',
        'boost_iostreams-mt-s',
        'boost_python-mt-s',
        'boost_zlib-mt-s',
        ]
    library_dirs=[boost_librarydir]
    extra_compile_args.append('-DBOOST_PYTHON_STATIC_LIB')
else:
    include_dirs = ['./wsgi_boost']
    libraries=[
        'boost_regex',
        'boost_system',
        'boost_thread',
        'boost_coroutine',
        'boost_context',
        'boost_filesystem',
        'boost_date_time',
        'boost_iostreams',
        'boost_python',
        'z',
        ]
    library_dirs=[]

files = [
    './wsgi_boost/connection.cpp',
    './wsgi_boost/request.cpp',
    './wsgi_boost/request_handlers.cpp',
    './wsgi_boost/response.cpp',
    './wsgi_boost/server.cpp',
    './wsgi_boost/wsgi_boost.cpp',
    ]

setup(name='wsgi_boost',
      ext_modules=[
          Extension('wsgi_boost',
                    files,
                    library_dirs=library_dirs,
                    libraries=libraries,
                    include_dirs=include_dirs,
                    extra_compile_args=extra_compile_args,
                    depends=[])
          ]
      )
