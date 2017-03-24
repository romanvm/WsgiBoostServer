#!/usr/bin/env python

import os
import sys
import re
from codecs import open
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

for item in sys.argv:
    if '--boost-headers' in item:
        os.environ['BOOST_ROOT'] = item.split('=')[1]
        sys.argv.remove(item)
for item in sys.argv:
    if '--boost-libs' in item:
        os.environ['BOOST_LIBRARYDIR'] = item.split('=')[1]
        sys.argv.remove(item)


class BuildError(Exception):
    pass


def patch_msvc_compiler():
    """
    Monkey-patch distutils to use MS Visual C++ 2015+ compiler
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
    with open(os.path.join(src, 'constants.h'), 'r') as fo:
        text = fo.read()
    return re.search(r'#define WSGI_BOOST_VERSION "(\d+\.\d+\.\d+[a-z]*)"', text, re.I).group(1)


def get_file(filename):
    with open(os.path.join(cwd, filename), 'r', encoding='utf-8') as fo:
        return fo.read()


sources = [os.path.join(src, file_) for file_ in os.listdir(src) if os.path.splitext(file_)[1] == '.cpp']
include_dirs = [src]
libraries = []
library_dirs=[]

define_macros = [('BOOST_COROUTINES_NO_DEPRECATION_WARNING', None)]
extra_compile_args = []
extra_link_args = []

if sys.platform == 'win32':
    patch_msvc_compiler()

    try:
        include_dirs.append(os.path.expandvars(os.environ['BOOST_ROOT']))
    except KeyError:
        raise BuildError('Path to Boost headers is not set! Use --boost-headers="<path>" option.')

    try:
        library_dirs.append(os.path.expandvars(os.environ['BOOST_LIBRARYDIR']))
    except KeyError:
        raise BuildError('Path to Boost libraries is not set! Use --boost-libs="<path>" option.')

    define_macros.append(('BOOST_PYTHON_STATIC_LIB', None))
    extra_compile_args.append('/EHsk')
    extra_compile_args.append('/MT')
    extra_link_args.append('/SAFESEH:NO')
else:
    try:
        include_dirs.append(os.path.expandvars(os.environ['BOOST_ROOT']))
    except KeyError:
        pass
    try:
        library_dirs.append(os.path.expandvars(os.environ['BOOST_LIBRARYDIR']))
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
            'boost_thread',
            'z',
        ]

    extra_compile_args.append('-std=c++11')


setup(
    name='WsgiBoostServer',
    version=get_version(),
    description='Asynchronous multithreaded HTTP/WSGI server based on C++ Boost.Asio',
    long_description=get_file('Readme.rst') + '\n\n' + get_file('Changelog.rst'),
    author='Roman Miroshnychenko',
    author_email='romanvm@yandex.ua',
    url='https://github.com/romanvm/WsgiBoostServer',
    license='MIT',
    keywords='boost asio wsgi server http',
    classifiers=[
        'Development Status :: 5 - Production/Stable',
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
    test_suite = 'test_wsgi_boost',
    tests_require=['requests'],
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
