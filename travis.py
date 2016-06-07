#!/usr/bin/env python

import os
import sys
import re

config = ['using gcc : 5 : g++-5 ;\n']
if len(sys.argv) > 1:
    py_version = re.search(r'^(\d+\.\d+)', sys.argv[1]).group(1)
    config.append('using python : {0} ;\n'.format(py_version))

with open(os.path.expanduser('~/user-config.jam'), 'w') as fo:
    fo.writelines(config)
