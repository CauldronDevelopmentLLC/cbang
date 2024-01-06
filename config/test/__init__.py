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

from SCons.Script import *
import shlex


def run_tests(env):
    import shlex
    import subprocess
    import sys

    cmd = shlex.split(env.get('TEST_COMMAND'))
    print('Executing:', cmd)
    sys.exit(subprocess.call(cmd))


def generate(env):
    import os
    import sys

    python = sys.executable

    dir = os.path.dirname(os.path.realpath(__file__))
    testHarness = dir + '/../../tests/testHarness'

    if env['PLATFORM'] == 'win32':
        python = python.replace('\\', '\\\\')
        testHarness = testHarness.replace('\\', '\\\\')

    cmd = [python, testHarness, '-C', 'tests', '--diff-failed', '--view-failed',
           '--view-unfiltered', '--save-failed', '--build']
    cmd = ' '.join([shlex.quote(s) for s in cmd]) # shlex.join() not in < 3.8

    env.CBAddVariables(('TEST_COMMAND', '`test` target command line', cmd))

    if 'test' in COMMAND_LINE_TARGETS: env.CBAddConfigureCB(run_tests)


def exists(): return 1
