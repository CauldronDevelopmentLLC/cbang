################################################################################
#                                                                              #
#         This file is part of the C! library.  A.K.A the cbang library.       #
#                                                                              #
#               Copyright (c) 2021-2026, Cauldron Development  Oy              #
#               Copyright (c) 2003-2021, Cauldron Development LLC              #
#                              All rights reserved.                            #
#                                                                              #
#        The C! library is free software: you can redistribute it and/or       #
#       modify it under the terms of the GNU Lesser General Public License     #
#      as published by the Free Software Foundation, either version 2.1 of     #
#              the License, or (at your option) any later version.             #
#                                                                              #
#       The C! library is distributed in the hope that it will be useful,      #
#         but WITHOUT ANY WARRANTY; without even the implied warranty of       #
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      #
#                Lesser General Public License for more details.               #
#                                                                              #
#        You should have received a copy of the GNU Lesser General Public      #
#                License along with the C! library.  If not, see               #
#                        <http://www.gnu.org/licenses/>.                       #
#                                                                              #
#       In addition, BSD licensing may be granted on a case by case basis      #
#       by written permission from at least one of the copyright holders.      #
#          You may request written permission by emailing the authors.         #
#                                                                              #
#                 For information regarding this software email:               #
#                                Joseph Coffland                               #
#                         joseph@cauldrondevelopment.com                       #
#                                                                              #
################################################################################

import os
import sys
import re
import subprocess
import glob

from SCons.Script import *


default_exclude = set([re.compile(r'^.*\\system32\\.*$')])


def find_in_path(filename):
    for path in os.environ["PATH"].split(os.pathsep):

        candidate = os.path.join(path.strip('"'), filename)
        if os.path.isfile(candidate): return candidate
        if not os.path.isdir(path): continue

        for name in os.listdir(path):
            if name.lower() == filename.lower():
                return os.path.join(path, name)


def find_dlls(env, path, exclude = set()):
    if env['PLATFORM'] == 'win32':
        prog = env.get('FIND_DLLS_DUMPBIN')
        cmd = [prog, '/DEPENDENTS', '/NOLOGO', path]

    else:
        prog = env.get('FIND_DLLS_OBJDUMP')
        cmd = [prog, '-p', path]


    p = subprocess.Popen(cmd, stdout = subprocess.PIPE)
    out, err = p.communicate()
    if isinstance(out, bytes): out = out.decode()
    if isinstance(err, bytes): err = err.decode()

    if p.returncode:
        raise Exception('Command failed: %s %s' % (cmd, err))

    for line in out.splitlines():
        if line.startswith('\tDLL Name: '): lib = line[11:]
        else:
            m = re.match(r'^\s+([^\s.]+\.[dD][lL][lL])\s*$', line)
            if m: lib = m.group(1)
            else: continue

        lib = lib.strip().lower()

        if not lib in exclude:
            exclude.add(lib)

            path = find_in_path(lib)
            if path is None:
                if env.get('FIND_DLLS_IGNORE_MISSING'): continue
                raise Exception('Lib "%s" not found' % lib)

            ok = True
            for pat in exclude:
                if isinstance(pat, type(re.compile(''))) and pat.match(path):
                    ok = False
                    break

            if not ok: continue

            yield path
            for path in find_dlls(env, path, exclude):
                yield path


def FindDLLs(env, source):
    if env.get('FIND_DLLS_DEFAULT_EXCLUDES'): exclude = set(default_exclude)
    else: exclude = set()

    for src in source:
        for path in glob.glob(env.subst(str(src))):
            for dll in find_dlls(env, path, exclude):
                yield dll


def generate(env):
    env.SetDefault(FIND_DLLS_DEFAULT_EXCLUDES = True)
    env.SetDefault(FIND_DLLS_IGNORE_MISSING = True)
    env.SetDefault(FIND_DLLS_OBJDUMP = 'objdump')
    env.SetDefault(FIND_DLLS_DUMPBIN = 'dumpbin')

    env.AddMethod(FindDLLs)


def exists():
    return True
