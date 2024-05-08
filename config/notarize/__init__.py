################################################################################
#                                                                              #
#         This file is part of the C! library.  A.K.A the cbang library.       #
#                                                                              #
#               Copyright (c) 2021-2024, Cauldron Development  Oy              #
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

''' notarize '''

import os
import sys
import subprocess
import re
import time
try:
    import json
except ImportError:
    import simplejson as json

from SCons.Script import *

deps = ['codesign'] # uses some sign_ vars


def notarize_sanity_check(env, fpath):
    if env.get('notarize_disable'):
        print('NOTE: notarize_disable is True', file=sys.stderr)
        return
    if env.get('sign_disable'):
        env['notarize_disable'] = True
        print('WARNING: setting notarize_disable because sign_disable is set',
              file=sys.stderr)
        return
    if not os.path.isfile(fpath):
        raise Exception('ERROR: file does not exist: \"%s\"' % fpath)
    # if invaid config disable notarization and print warning
    notarize_profile = env.get('notarize_profile')
    if not notarize_profile:
        env['notarize_disable'] = True
        print('WARNING: notarize_profile is not set', file=sys.stderr)
        print('WARNING: setting notarize_disable True', file=sys.stderr)
    if not env.get('sign_prefix'):
        print('WARNING: sign_prefix is not set', file=sys.stderr)


def notarize_staple_file(fpath):
    cmd = ['/usr/bin/xcrun', 'stapler', 'staple', fpath]
    print('Stapling ' + fpath)
    # Maybe check exit code; loop if fail until timeout
    # Note: failure to staple may not be fatal, assuming
    # the notarization request will be approved sometime later.
    # Stapling just allows offline validation by Gatekeeper.
    # It would be bad for staple to fail when status was "success".
    ret = subprocess.call(cmd)
    if ret != 0:
        print('WARNING: unable to staple ' + fpath, file=sys.stderr)
    return ret


def NotarizeWaitStaple(env, fpath, timeout=0):
    notarize_sanity_check(env, fpath)
    if env.get('sign_disable'): return False
    if env.get('notarize_disable'): return False
    if not env.get('notarize_profile'): return False
    if timeout < 0: timeout = 0
    elif timeout > 3600: timeout = 3600
    notarize_profile = env.get('notarize_profile')
    cmd = ['/usr/bin/xcrun', 'notarytool', 'submit', '--wait']
    if timeout: cmd += ['--timeout', str(timeout)]
    cmd += ['-p', notarize_profile, fpath]
    print('@', cmd)
    ret = subprocess.call(cmd)
    if ret != 0:
        raise Exception('ERROR: unable to notarize \"%s\"' % fpath)
    ret = notarize_staple_file(fpath)
    if ret != 0:
        print('ERROR: Stapling failed, though notarize may have succeeded',
              file=sys.stderr)
    print('Done notarizing.')
    return ret == 0


def generate(env):
    if env['PLATFORM'] == 'darwin' or int(env.get('cross_osx', 0)):
        env.CBAddVariables(
            # set these vars in scons-options.py
            # sign_prefix is prepended tp pkg filename to make identifier
            # You should at least disable notarization for debug builds
            # Really, only pkgs users can download need be notarized
            BoolVariable('notarize_disable', 'Disable notarization', 0),
            ('notarize_profile', 'The notarytool keychain item profile name'),
            # if using notarize_profile, you do not need user, pass, asc, team
            ('notarize_user', 'The deveoper AppleID email used to notarize.' +
             ' Need not be same as codesign account.'),
            # The 10-character asc and team are allegedly optional if
            # there is only one developer in notarize_user account.
            # If both are specified, only asc is used.
            ('notarize_asc', 'The developer asc provider id'),
            ('notarize_team', 'The developer team id'),
            )

        env.AddMethod(NotarizeWaitStaple)

    for tool in deps:
        env.CBLoadTool(tool)

    return True


def exists():
    return 1
