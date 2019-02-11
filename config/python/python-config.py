#!env python
from __future__ import print_function

import sys
import os
import getopt
from distutils import sysconfig


vars = sysconfig.get_config_vars()
pyver = vars['VERSION']

flags = ['-I' + sysconfig.get_python_inc(),
         '-I' + sysconfig.get_python_inc(plat_specific = True)]
print(' '.join(flags))

libs = vars['LIBS'].split() + vars['SYSLIBS'].split()

if 'BLDLIBRARY' in vars: libs.append(vars['BLDLIBRARY'])
else: libs.append('-lpython' + pyver)
if not vars['Py_ENABLE_SHARED']:
    libs.insert(0, '-L' + vars['LIBPL'])
print(' '.join(libs))
