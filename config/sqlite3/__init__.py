################################################################################
#                                                                              #
#         This file is part of the C! library.  A.K.A the cbang library.       #
#                                                                              #
#               Copyright (c) 2021-2025, Cauldron Development  Oy              #
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

from SCons.Script import *


def configure(conf):
    env = conf.env

    if env['PLATFORM'] == 'win32' or int(env.get('cross_mingw', 0)):
        if not conf.CBCheckLib('ws2_32'): conf.CBRequireLib('wsock32')
    else:
        conf.CBConfig('pthreads')
        conf.CBCheckLib('dl')

    conf.CBCheckHome('libsqlite', ['', '/include'], ['', '/lib'])
    conf.CBRequireLib('sqlite3')
    conf.CBRequireHeader('sqlite3.h')
    conf.CBRequireFunc('sqlite3_backup_init')

    env.CBConfigDef('HAVE_LIBSQLITE')


def generate(env):
    env.CBAddConfigTest('sqlite3', configure)
    env.CBLoadTool('pthreads')


def exists():
    return 1
