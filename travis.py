#!/usr/bin/env python

import os
import sys

config = ['using gcc : 5 : g++-5 ;\n']
if len(sys.argv) > 1:
    config.append('using python : {0} ;\n'.format(sys.argv[1]))

with open(os.path.expanduser('~/user-config.jam'), 'w') as fo:
    fo.writelines(config)
