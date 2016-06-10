#!/usr/bin/env python
"""
Writes user-config.jam for Boost.Build
"""

import os
import re

py_version = re.search(r'^(\d+\.\d+)', os.environ['TRAVIS_PYTHON_VERSION']).group(1)
config = [
    'using gcc : 5 : g++-5 ;\n',
    'using python : {0} : /usr/bin/python{0} : /usr/include/python{0} : /usr/lib/x86_64-linux-gnu ;\n'.format(py_version),
    ]
with open(os.path.expanduser('~/user-config.jam'), 'w') as fo:
    fo.writelines(config)
