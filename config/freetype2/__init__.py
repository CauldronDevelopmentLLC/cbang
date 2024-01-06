################################################################################
#                                                                              #
#         This file is part of the C! library.  A.K.A the cbang library.       #
#                                                                              #
#               Copyright (c) 2003-2024, Cauldron Development LLC              #
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


def configure(conf):
    env = conf.env

    conf.CBCheckHome('freetype2',
                     inc_suffix=['/include', '/include/freetype2'])

    if not 'FREETYPE2_INCLUDE' in os.environ:
        try:
            env.ParseConfig('pkg-config freetype2 --cflags')
        except OSError:
            try:
                env.ParseConfig('freetype-config --cflags')
            except OSError:
                pass


    if env['PLATFORM'] == 'darwin' or int(env.get('cross_osx', 0)):
        if not conf.CheckOSXFramework('CoreServices'):
            raise Exception('Need CoreServices framework')

    conf.CBRequireCHeader('ft2build.h')
    conf.CBRequireLib('freetype')
    conf.CBConfig('ZLib')
    conf.CBCheckLib('png')
    conf.CBCheckLib('brotlidec')
    conf.CBCheckLib('brotlicommon')

    return True


def generate(env):
    env.CBAddConfigTest('freetype2', configure)
    env.CBLoadTools('osx ZLib')


def exists():
    return 1
