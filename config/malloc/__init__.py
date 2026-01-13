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
from SCons.Script import *


def configure(conf, cxx = True, threads = True):
    env = conf.env

    # See http://dmalloc.com/
    if env.get('dmalloc'):
        libname = 'dmalloc'

        if threads: libname += 'th'
        if cxx: libname += 'cxx'

        if cxx: lang = 'C++'
        else: lang = None

        conf.CBRequireLib(libname, language = lang)


    # See http://linux.die.net/man/3/efence
    if env.get('efence'): conf.CBRequireLib('efence')


    # See http://code.google.com/p/google-perftools
    if env.get('tcmalloc'):
        if conf.CBCheckCXXHeader('google/tcmalloc.h'):
            env.CBDefine('HAVE_TCMALLOC_H')

        libname = 'tcmalloc'
        #if env.get('debug'): libname += '_debug'

        conf.CBRequireLib(libname)
        env.Append(PREFER_DYNAMIC = [libname])

        env.AppendUnique(CCFLAGS = [
                '-fno-builtin-malloc', '-fno-builtin-calloc',
                '-fno-builtin-realloc', '-fno-builtin-free'])


    # See http://libcwd.sourceforge.net/
    if env.get('cwd'):
        libname = 'cwd'
        if threads:
            libname += '_r'
            env.CBDefine('LIBCWD_THREAD_SAFE')

        if env.get('debug'): env.CBDefine('CWDEBUG')

        if conf.CBCheckCXXHeader('libcwd/sys.h'):
            env.CBDefine('HAVE_LIBCWD')

        conf.CBRequireLib(libname)

        # libcwd does not work if libdl is included anywhere earlier
        conf.CBRequireLib('dl')


def generate(env):
    env.CBAddConfigTest('malloc', configure)

    env.CBAddVariables(
        ('dmalloc', 'Compile with dmalloc', 0),
        ('tcmalloc', 'Compile with google perfomrance tools malloc', 0),
        ('cwd', 'Compile with libcwd', 0),
        ('efence', 'Compile with electric-fence', 0))


def exists():
    return 1
