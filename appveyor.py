#!/usr/bin/env python
"""
Writes user-config.jam for Boost.Build
"""

import os

config = [
    'using msvc : 14.0 ;\n',
    'using python : {0} : python :  {1}/include : {1}/libs ;\n'.format(
        os.environ['PYTHON_VERSION'],
        os.environ['PYTHON'].replace('\\', '/')
        )
    ]

with open(os.path.join(os.path.expandvars('%USERPROFILE%'), 'user-config.jam'), 'w') as fo:
    fo.writelines(config)
