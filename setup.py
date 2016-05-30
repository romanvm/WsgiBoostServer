#!/usr/bin/env python

import os
import sys
from distutils.msvc9compiler import get_build_version
try:
    from setuptools import setup
    from setuptools.extension import Extension
except ImportError:
    from distutils.core import setup
    from distutils.extension import Extension


class BuildError(Exception):
    pass


files = [
    './wsgi_boost/connection.cpp',
    './wsgi_boost/request.cpp',
    './wsgi_boost/request_handlers.cpp',
    './wsgi_boost/response.cpp',
    './wsgi_boost/server.cpp',
    './wsgi_boost/wsgi_boost.cpp',
    ]
include_dirs = ['./wsgi_boost']
libraries = [
    'boost_regex',
    'boost_system',
    'boost_thread',
    'boost_coroutine',
    'boost_context',
    'boost_filesystem',
    'boost_date_time',
    'boost_iostreams',
    'boost_python',
    ]
define_macros = [('BOOST_ALL_NO_LIB', None)]
library_dirs=[]
extra_compile_args = []
extra_link_args = []
if sys.platform == 'win32':
    # Monkey-patching the default distutils compliler to MSVS 2015
    vs_comntools = 'VS{0}COMNTOOLS'.format(int(get_build_version() * 10))
    try:
        os.environ[vs_comntools] = os.environ['VS140COMNTOOLS']
    except KeyError:
        raise BuildError('Compatible compiler not found! Only MSVS 2015 Update 2+ is supported.')
    try:
        boost_root = os.environ['BOOST_ROOT']
    except KeyError:
        raise BuildError('BOOST_ROOT environment variable is not defined!')
    include_dirs.append(boost_root)
    try:
        boost_librarydir = os.environ['BOOST_LIBRARYDIR']
    except KeyError:
        raise BuildError('BOOST_LIBRARYDIR environment variable is not defined!')
    library_dirs.append(boost_librarydir)
    libraries.append('libboost_zlib')
    libs = os.listdir(boost_librarydir)
    for i, item in enumerate(libraries):
        for lib in libs:
            if item in lib:
                libraries[i] = os.path.splitext(lib)[0]
                break
        else:
           raise BuildError('Unable to find "{0}" library!'.format(item))
    define_macros.append(('BOOST_PYTHON_STATIC_LIB', None))
    extra_compile_args.append('/EHsk')
    extra_compile_args.append('/MT')
    extra_link_args.append('/SAFESEH:NO')
else:
    libraries.append('z')
    extra_compile_args.append('-std=c++11')

setup(name='wsgi_boost',
      ext_modules=[
          Extension('wsgi_boost',
                    files,
                    library_dirs=library_dirs,
                    libraries=libraries,
                    include_dirs=include_dirs,
                    define_macros=define_macros,
                    extra_compile_args=extra_compile_args,
                    extra_link_args=extra_link_args,
                    depends=[])
          ]
      )
