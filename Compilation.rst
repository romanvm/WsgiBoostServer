Compilation
===========

Normally, WsgiBoostServer is compiled using ``setup.py`` script.

Linux
-----

WsgiBoostServer can be compiled on any Linux distributive that uses GCC 4.9+ as the default system compiler,
for example Ubuntu 16.04 LTS (Xenial Xerus). The procedure below assumes that you are using Ubuntu 16.04.

Note for **Raspberry Pi** users: Raspbian provides GCC 4.9 compiler by default and its repositories
have all the necessary libraries, so if you want to compile WsgiBoostServer for Raspberry Pi
the procedure below can be used on Raspbian with no changes.
On **Releases** tab of this repository you can find a statically compiled wheel with HTTPS support
for Python 3.4 on Raspberry Pi 2.

Python and Boost From System Repositories
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following procedure assumes that you are using Python and Boost from system repositories. For example,
Ubuntu 16.04 LTS (Xenial Xerus) provides Python 3.5 and Boost 1.58.

- Install prerequisites::

    $ sudo apt-get install build-essential python3-dev python3-pip zlib1g-dev libbz2-dev libssl-dev
    $ sudo apt-get install libboost-regex-dev libboost-system-dev libboost-coroutine-dev libboost-context-dev libboost-filesystem-dev libboost-iostreams-dev libboost-date-time-dev libboost-thread-dev

- Download or clone WsgiBoostServer sources to your computer.

- Activate your working Python virtual environment (optional).

- Build WsgiBoostServer::

    $ python3 setup.py build

- Run tests (optional)::

    $ python3 setup.py test

- Install WsgiBoostServer::

    $ python3 setup.py install

  It is strongly recommended to use Python virtual environments.

Alternatively, after installing prerequisites you can install WsgiBoostServer directly from GitHub repository
using ``pip``::

  $ pip3 install git+https://github.com/romanvm/WsgiBoostServer.git#egg=WsgiBoostServer

``pip`` will download WsgiBoostServer sources, compile the binary module
and install it into your working Python environment.

Custom Python and/or Boost Versions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following procedure assumes that you want to use a custom Boost version.
In WsgiBoostServer v.2.0 Python bindings were converted to Pybind11 library that allowed to remove
dependency on Boost.Python library pre-built against a specific Python version.
Now WsgiBoostServer uses only generic Boost libraries.

- Install prerequisites::

    $ sudo apt-get install build-essential python3-pip zlib1g-dev libbz2-dev libssl-dev

- Install the necessary Python development package. For example, `deadsnakes`_ ppa repository provides various
  Python versions for Ubuntu. Let's assume we want to build WsgiBoostServer with Python 3.6::

    $ sudo apt-get install software-properties-common python-software-properties 
    $ sudo add-apt-repository ppa:fkrull/deadsnakes
    $ sudo apt-get update
    $ sudo apt-get install python3.6 python3.6-dev

  Now if you want to use only a custom Python version, follow the preceding procedure but
  replace python3 with python3.6 (you need to install full prerequisites except for python3-dev).

- Downloaded the necessary Boost version from `Boost Home Site`_ and unpack it to a folder
  of your choice, for example ``$HOME/boost``.

- Go to Boost folder and run::

    $ sh bootstrap.sh

- Run in the Boost folder::

    $ ./b2 link=static variant=release cxxflags=-fPIC --layout=system --with-regex --with-system --with-coroutine --with-context --with-filesystem --with-iostreams --with-date_time

  Boost.Build engine will build the necessary libraries to link WsgiBoostServer against and place them into
  ``$HOME/boost/stage/lib`` folder. This folder can be changed by the ``--stagedir=`` option.

- Build WsgiBoostServer using ``setup.py`` script::

    $ python3.6 setup.py build --boost-headers="$HOME/boost" --boost-libs="$HOME/boost/stage/lib"

  The ``--boost-headers=`` and ``--boost-libs=`` options must point to actual folders where Boost header files and libraries are located.

- Install WsgiBoostServer::
  
    $ python3.6 setup.py install

  It is strongly recommended to use Python virtual environments.

HTTPS Support
~~~~~~~~~~~~~

By default, on Linux WsgiBoostServer is built with HTTPS support. You can disable it by providing
``--without-ssl`` option to ``setup.py`` script.

Windows
-------

Compiled wheels for Python 3.6 with HTTPS support can be downloaded from "**Releases**" section of this repository.
If you want to compile WsgiBoostServer for Windows yourself, follow the instruction below.
You can also check AppVeyor CI build configuration ``appveyor.yml``.

**Tools required**: MS Visual Studio 2015 Update 3+, Cmake

Note that on Windows you can build WsgiBoostServer only for Python 3 versions that have been compiled
with MS Visual Studio 2015 and above, that is, versions 3.5 and newer.

Procedure
~~~~~~~~~

The build procedure is similar to that for custom Python and Boost versions on Linux.
Again, WsgiBoostServer no longer requires linking against a version-specific Boost.Python library
and uses only generic Boost libraries.

- Download ``zlib`` sources from `zlib Home Site`_ and unpack them into the folder of your choice,
  for example ``c:\zlib``.

- Open console, go to the ``zlib`` folder and execute there::

    >cmake .

  You don't need to compile ``zlib``, ``Boost.Buld`` will do that for you.

- Optionally, for HTTPS support you can download and install `OpenSSL for Windows`_ (full package).

- Download ``boost`` sources from `Boost Home Site`_  and unpack them into the folder of your choice,
  for example ``c:\boost``.

- Open Windows console, go to the ``boost`` folder and execute there::

    >bootstrap

- Execute in the ``boost`` folder::

    >b2 link=static runtime-link=static variant=release -sZLIB_SOURCE=c:\zlib --with-regex --with-system --with-coroutine --with-context --with-filesystem --with-iostreams --with-date_time

  Note that ``-sZLIB_SOURCE`` option should point to your actual ``zlib`` folder.

  Boost.Build engine will build the necessary libraries to link WsgiBoostServer against and place them into
  ``c:\boost\stage\lib`` folder. This folder can be changed by the ``--stagedir=`` option.

- Build WsgiBoostServer using ``setup.py`` script::

    >python setup.py build --boost-headers="c:\boost" --boost-libs="c:\boost\stage\lib"

  The ``--boost-headers=`` and ``--boost-libs=`` options must point to the actual folders where Boost header files and libraries are located.
  Note that you must use the same Python version that was used to build Boost.Python library.

  Optionally, for HTTPS support you can also provide the path to OpenSSL libraries with ``--open-ssl-dir=`` option,
  for example::

    >python setup.py build --boost-headers="c:\boost" --boost-libs="c:\boost\stage\lib" --open-ssl-dir="c:\OpenSSL-Win32"

- Install WsgiBoostServer::

    >python setup.py install

  It is strongly recommended to use Python virtual environments.

.. _zlib Home Site: http://www.zlib.net
.. _Boost Home Site: http://www.boost.org
.. _deadsnakes: https://launchpad.net/~fkrull/+archive/ubuntu/deadsnakes
.. _Boost.Build documentation: http://www.boost.org/doc/libs/1_63_0/libs/python/doc/html/building/configuring_boost_build.html
.. _OpenSSL for Windows: https://slproweb.com/products/Win32OpenSSL.html
