#!/usr/bin/env python

from __future__ import print_function
import os
import sys
import re
from setuptools import setup
from setuptools.extension import Extension
try:
    import distutils.msvc9compiler
except ImportError:
    pass

NAME = 'wsgi_boost'

SUPPORTED_MSVC_COMPILERS = [14.0]

cwd = os.path.dirname(os.path.abspath(__file__))
src = os.path.join(cwd, NAME)


class BuildError(Exception):
    pass


def patch_msvc_compiler():
    """
    Monkey-patch distutils to use MS Visual C++ 2015 compiler
    """
    for vc_version in SUPPORTED_MSVC_COMPILERS:
        vcvarsall = distutils.msvc9compiler.find_vcvarsall(vc_version)
        if vcvarsall is not None:
            distutils.msvc9compiler.get_build_version = lambda: vc_version
            distutils.msvc9compiler.find_vcvarsall = lambda version: vcvarsall
            break
    else:
        raise BuildError('Compatible MS Visual C++ compiler not found! Only v. {0} are supported.'.format(str(SUPPORTED_MSVC_COMPILERS)))


def get_version():
    with open(os.path.join(src, 'response.h'), 'r') as fo:
        response_h = fo.read()
    return re.search(r'#define WSGI_BOOST_VERSION "(\d+\.\d+\.\d+)"', response_h).group(1)


def get_long_description():
    with open(os.path.join(cwd, 'Readme.rst'), 'r') as fo:
        return fo.read()


sources = [os.path.join(src, file) for file in os.listdir(src) if os.path.splitext(file)[1] == '.cpp']
include_dirs = [src]
libraries = []
library_dirs=[]

define_macros = []
extra_compile_args = []
extra_link_args = []

if sys.platform == 'win32':
    patch_msvc_compiler()

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

    define_macros.append(('BOOST_PYTHON_STATIC_LIB', None))
    extra_compile_args.append('/EHsk')
    extra_compile_args.append('/MT')
    extra_link_args.append('/SAFESEH:NO')
else:
    try:
        include_dirs.append(os.environ['BOOST_ROOT'])
    except KeyError:
        pass
    try:
        library_dirs.append(os.environ['BOOST_LIBRARYDIR'])
    except KeyError:
        libraries.append(
            'boost_python-py{major}{minor}'.format(
                major=sys.version_info.major,
                minor=sys.version_info.minor
                ))
    else:
        libraries.append('boost_python')
    libraries += [
            'boost_regex',
            'boost_system',
            'boost_coroutine',
            'boost_context',
            'boost_filesystem',
            'boost_iostreams',
            'boost_date_time',
            'z',
        ]

    extra_compile_args.append('-std=c++11')

setup(
    name='WsgiBoostServer',
    version=get_version(),
    description='Multithreaded WSGI server based on C++ Boost.Asio',
    long_description=get_long_description(),
    author='Roman Miroshnychenko',
    author_email='romanvm@yandex.ua',
    url='https://github.com/romanvm/WsgiBoostServer',
    license='MIT',
    keywords='boost asio wsgi server http',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Environment :: Web Environment',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Operating System :: Microsoft',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: C++',
        'Programming Language :: Python',
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 3',
        'Topic :: Internet :: WWW/HTTP',
        'Topic :: Internet :: WWW/HTTP :: WSGI',
        'Topic :: Internet :: WWW/HTTP :: WSGI :: Server',
        ],
    platforms=['win32', 'posix'],
    zip_safe=False,
    test_suite = 'nose.collector',
    tests_require=['nose'],
    ext_modules=[
        Extension(
            name=NAME,
            sources=sources,
            library_dirs=library_dirs,
            libraries=libraries,
            include_dirs=include_dirs,
            define_macros=define_macros,
            extra_compile_args=extra_compile_args,
            extra_link_args=extra_link_args,
            depends=[]
            )
        ]
    )
