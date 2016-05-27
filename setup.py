#!/usr/bin/env python

import sys
from distutils.core import setup
from distutils.extension import Extension


if sys.platform == 'win32' :
    include_dirs = ['d:/boost_1_60_0', './wsgi_boost']
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
        'boost_zlib',
        ]
    library_dirs=['d:/boost_1_60_0/']
else :
    include_dirs = ['/usr/include/boost', './wsgi_boost']
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
    library_dirs=['/usr/lib/i386-linux-gnu']

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
                              depends=[]),
                    ]
     )
