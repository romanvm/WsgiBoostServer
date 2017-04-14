#!/usr/bin/env python

import os
import sys
import re
from codecs import open
from setuptools import setup
from setuptools.extension import Extension

if sys.version_info.major < 3:
    raise DeprecationWarning('Python 2 is not supported!')

NAME = 'wsgi_boost'

this_dir = os.path.dirname(os.path.abspath(__file__))
third_party = os.path.join(this_dir, 'third-party')
src = os.path.join(this_dir, NAME)

for item in sys.argv:
    if '--boost-headers' in item:
        os.environ['BOOST_ROOT'] = item.split('=')[1]
        sys.argv.remove(item)
        break
for item in sys.argv:
    if '--boost-libs' in item:
        os.environ['BOOST_LIBRARYDIR'] = item.split('=')[1]
        sys.argv.remove(item)
        break

ssl_enabled = True
for item in sys.argv:
    if '--without-ssl' in item:
        ssl_enabled = False
        sys.argv.remove(item)
        break
if ssl_enabled:
    for item in sys.argv:
        if '--open-ssl-dir' in item:
            os.environ['OPENSSL_ROOT_DIR'] = item.split('=')[1]
            sys.argv.remove(item)
            break
generate_pdb = False
for item in sys.argv:
    if '--debug' in item:
        generate_pdb = True
        sys.argv.remove(item)
        break


class BuildError(Exception):
    pass


def get_version():
    with open(os.path.join(src, 'constants.h'), 'r') as fo:
        text = fo.read()
    return re.search(r'#define WSGI_BOOST_VERSION "(\d+\.\d+\.\d+\w*)"', text, re.I).group(1)


def get_file(filename):
    with open(os.path.join(this_dir, filename), 'r', encoding='utf-8') as fo:
        return fo.read()


sources = [os.path.join(src, 'wsgi_boost.cpp')]
include_dirs = [third_party, src]
libraries = []
library_dirs=[]

define_macros = [('BOOST_COROUTINES_NO_DEPRECATION_WARNING', None)]
extra_compile_args = []
extra_link_args = []

if sys.platform == 'win32':

    try:
        include_dirs.append(os.path.expandvars(os.environ['BOOST_ROOT']))
    except KeyError:
        raise BuildError('Path to Boost headers is not set! Use --boost-headers="<path>" option.')

    try:
        library_dirs.append(os.path.expandvars(os.environ['BOOST_LIBRARYDIR']))
    except KeyError:
        raise BuildError('Path to Boost libraries is not set! Use --boost-libs="<path>" option.')

    if ssl_enabled:
        try:
            openssl_root = os.path.expandvars(os.environ['OPENSSL_ROOT_DIR'])
        except KeyError:
            pass
        else:
            define_macros.append(('HTTPS_ENABLED', None))
            include_dirs.append(os.path.join(openssl_root, 'include'))
            openssl_libs = os.path.join(openssl_root, 'lib', 'VC', 'static')
            library_dirs.append(openssl_libs)
            if os.path.exists(os.path.join(openssl_libs, 'libeay32MT.lib')):
                # OpenSSL v.1.0.x
                libraries += [
                    'libeay32MT',
                    'ssleay32MT',
                ]
            else:  # OpenSSL 1.1.x
                if sys.maxsize > 2 ** 32 // 2 - 1:
                    arch = 64
                else:
                    arch = 32
                libraries += [
                    'libcrypto{}MT'.format(arch),
                    'libssl{}MT'.format(arch),
                ]
            libraries += [
                'advapi32',
                'crypt32',
                'gdi32',
                'User32',
                'legacy_stdio_definitions',
                ]

    extra_compile_args.append('/EHsk')
    extra_compile_args.append('/MT')
    extra_link_args.append('/SAFESEH:NO')
    if generate_pdb:
        extra_link_args.append('/DEBUG')
else:
    try:
        include_dirs.append(os.path.expandvars(os.environ['BOOST_ROOT']))
    except KeyError:
        pass
    try:
        library_dirs.append(os.path.expandvars(os.environ['BOOST_LIBRARYDIR']))
    except KeyError:
        pass
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

    if ssl_enabled:
        define_macros.append(('HTTPS_ENABLED', None))
        libraries += ['crypto', 'ssl']
    extra_compile_args.append('-std=c++11')
    if generate_pdb:
        extra_compile_args.append('-ggdb')


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
        #'Development Status :: 5 - Production/Stable',
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
        'Programming Language :: Python :: 3',
        'Topic :: Internet :: WWW/HTTP',
        'Topic :: Internet :: WWW/HTTP :: WSGI',
        'Topic :: Internet :: WWW/HTTP :: WSGI :: Server',
        ],
    platforms=['win32', 'posix'],
    zip_safe=False,
    test_suite='test_wsgi_boost',
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
