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

import platform
from SCons.Script import *


def CheckOSXFramework(ctx, name):
    env = ctx.env

    if platform.system().lower() == 'darwin' or int(env.get('cross_osx', 0)):
        ctx.Message('Checking for framework %s... ' % name)
        save_FRAMEWORKS = env['FRAMEWORKS']
        env.PrependUnique(FRAMEWORKS = [name])
        result = \
            ctx.TryLink('int main() {return 0;}\n', '.c')
        ctx.Result(result)

        if not result:
            env.Replace(FRAMEWORKS = save_FRAMEWORKS)

        return result

    ctx.Result(False)
    return False


def RequireOSXFramework(ctx, name):
    if not ctx.sconf.CheckOSXFramework(name):
        raise Exception('Need Framework ' + name)


def generate(env):
    env.CBAddTest(CheckOSXFramework)
    env.CBAddTest(RequireOSXFramework)


def exists():
    return 1
