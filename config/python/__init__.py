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

from __future__ import print_function

from SCons.Script import *
import inspect
import traceback


def try_config(conf, command):
    env = conf.env

    try:
        env.ParseConfig(command)
        env.Prepend(LIBS = ['util', 'm', 'dl', 'z'])

    except OSError:
        return False

    if conf.CheckHeader('Python.h') and conf.CheckFunc('Py_Initialize'):
        env.ParseConfig(command)
        env.CBConfigDef('HAVE_PYTHON');
        env.Prepend(LIBS = ['util', 'm', 'dl', 'z'])

        return True

    return False


def configure(conf):
    env = conf.env

    if not env.get('python', 0): return False

    python_version = env.get('python_version', '')

    dir = os.path.dirname(inspect.getfile(inspect.currentframe()))
    cmd = "python%s '%s'/python-config.py" % (python_version, dir)

    return try_config(conf, cmd)


def generate(env):
    env.CBAddConfigTest('python', configure)

    env.CBAddVariables(
        BoolVariable('python', 'Set to 0 to disable python', 1),
        ('python_version', 'Set python version', ''))



def exists():
    return 1
