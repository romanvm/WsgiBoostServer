#!/usr/bin/env python

from __future__ import print_function
import os
import sys
import re
from setuptools import setup
from setuptools.extension import Extension

extension = 'wsgi_boost'

SUPPORTED_MSVC_COMPILERS = [14.0]

cwd = os.path.dirname(os.path.abspath(__file__))
src = os.path.join(cwd, extension)


class BuildError(Exception):
    pass


def get_msvc_compiler():
    """
    Returns environment path for a supported MS Visual C++ compiler
    """
    for vc_version in SUPPORTED_MSVC_COMPILERS:
        compiler = 'VS{0}COMNTOOLS'.format(int(vc_version * 10))
        if compiler in os.environ:
            return os.environ[compiler]
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

define_macros = [('BOOST_PYTHON_STATIC_LIB', None)]
extra_compile_args = []
extra_link_args = []

if sys.platform == 'win32':
    from distutils.msvc9compiler import get_build_version
    # Monkey-patching the default distutils compliler to a supported MS Visual C++ version
    vs_comntools = 'VS{0}COMNTOOLS'.format(int(get_build_version() * 10))
    os.environ[vs_comntools] = get_msvc_compiler()

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

    boost_python = 'boost_python{0}.*?-mt-s[\.-]'
    if sys.version_info.major >= 3:
        boost_python = boost_python.format(sys.version_info.major)
    else:
        boost_python = boost_python.format('')
    boost_py_re = re.compile(boost_python, re.IGNORECASE)
    for lib in os.listdir(boost_librarydir):
        if re.search(boost_py_re, lib) is not None:
            libraries.append(os.path.splitext(lib)[0])
            break
    else:
        raise BuildError('Unable to find boost_python library!')

    extra_compile_args.append('/EHsk')
    extra_compile_args.append('/MT')
    extra_link_args.append('/SAFESEH:NO')
else:
    libraries += [
    'boost_python-py{major}{minor}'.format(
        major=sys.version_info.major,
        minor=sys.version_info.minor
    ),
    'boost_regex',
    'boost_system',
    'boost_coroutine',
    'boost_context',
    'boost_filesystem',
    'boost_iostreams',
    'z',
    ]

    extra_compile_args.append('-std=c++11')
    extra_link_args.append('-Bdynamic')

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
    ext_modules=[
        Extension(
            name=extension,
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
