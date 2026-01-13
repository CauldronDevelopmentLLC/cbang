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

''' codesign - macOS codesign/productsign functions '''

import os, platform, shutil

from SCons.Script import *
from SCons.Action import CommandAction

''' TODO
Add builder Codesign(sources)
  prog = env.Program(...)
  sign = env.Codesign(prog)
  Default(sign)
  Depends(sign, prog)
'''

def UnlockKeychain(env, keychain=None, password=None):
    if keychain is None: keychain = env.get('sign_keychain')
    if keychain: name = keychain
    else: name = 'default-keychain'
    if password:
        cmd = ['security', 'unlock-keychain', '-p', password]
        if keychain: cmd += [keychain]
        try:
            sanitized_cmd = cmd[:3] + ['****']
            if keychain: sanitized_cmd += [keychain]
            print('@', sanitized_cmd)
            # returns 0 if keychain already unlocked, even if pass is wrong
            ret = CommandAction(cmd).execute(None, [], env)
            if ret: raise Exception(
                'unlock-keychain failed, return code %s' % str(ret))
            print('keychain "%s" is unlocked' % name)
        except Exception as e:
            print('unable to unlock keychain "%s"' % name)
            raise e
    else:
        print('skipping unlock "%s"' % name + '; no password given')


def SignExecutable(env, target):
    if env.get('sign_disable'): return
    keychain = env.get('sign_keychain')
    sign = env.get('sign_id_app')
    prefix = env.get('sign_prefix')
    cmd = ['codesign', '-f', '--timestamp', '--options', 'runtime']
    if keychain: cmd += ['--keychain', keychain]
    if prefix:
        if not prefix.endswith('.'): prefix += '.'
        cmd += ['--prefix', prefix]
    else:
        raise Exception('unable to codesign %s; no sign_prefix given' % target)
    if sign: cmd += ['--sign', sign]
    else:
        raise Exception('unable to codesign %s; no sign_id_app given' % target)
    if not (os.path.isfile(target) and os.access(target, os.X_OK)):
        raise Exception('unable to codesign %s; not an executable' % target)
    # FIXME should not try to sign executable scripts
    cmd += [target]
    env.RunCommandOrRaise(cmd)


def SignApplication(env, target):
    if env.get('sign_disable'): return
    keychain = env.get('sign_keychain')
    sign = env.get('sign_id_app')
    cmd = ['codesign', '-f', '--timestamp', '--options', 'runtime']
    try:
      ver = tuple([int(x) for x in platform.mac_ver()[0].split('.')])
      # all bundles in an app must also be signed on 10.7+
      # easy way to do this is just to codesign --deep
      # only ok if there are no sandbox entitlements for this app
      if ver >= (10,7): cmd += ['--deep']
    except: pass
    if keychain: cmd += ['--keychain', keychain]
    if sign: cmd += ['--sign', sign]
    else:
        raise Exception('unable to codesign %s; no sign_id_app given' % target)
    if not os.path.isdir(target) or not target.endswith('.app'):
        raise Exception('unable to codesign %s; not an app' % target)
    epath = os.path.join(target, 'Contents/Resources/entitlements.plist')
    if os.path.isfile(epath): cmd += ['--entitlements', epath]
    cmd += [target]
    # sign any bundled libraries before singning app
    # https://github.com/FoldingAtHome/fah-issues/issues/1576
    # find libs under target dir, ignoring symlinks
    paths = []
    for root, dirs, files in os.walk(target):
        for file in files:
            e = os.path.splitext(file)[1]
            if e in ('.so', '.dylib'):
                path = os.path.join(root, file)
                if not os.path.islink(path):
                    paths.append(path)
    # sign each lib
    for path in paths:
        # some macports built libs are missing execute bits
        if not os.access(path, os.X_OK):
            print ('warning: setting execute perms on ' + path)
            os.chmod(path, 0o755)
        SignExecutable(env, path)
    # finally, sign app
    env.RunCommandOrRaise(cmd)


def SignPackage(env, target, source):
    # expect source 'somewhere/tmpname.pkg', target elsewhere/finalname.pkg
    if env.get('sign_disable'): return
    sign = env.get('sign_id_installer')
    keychain = env.get('sign_keychain')
    if not sign:
        raise Exception('unable to sign; no sign_id_installer provided')
    # FIXME should do more than this to verify flat pkg
    x, ext = os.path.splitext(source)
    if not (os.path.isfile(source) and ext in ('.pkg', '.mpkg')):
        raise Exception('unable to sign; not a flat package: ' + source)
    cmd = ['productsign','--timestamp', '--sign', sign]
    if keychain: cmd += ['--keychain', keychain]
    cmd += [source, target]
    env.RunCommandOrRaise(cmd)


def SignOrCopyPackage(env, target, source):
    if env.get('sign_id_installer') and not env.get('sign_disable'):
        SignPackage(env, target, source)
        return
    print('WARNING: NOT signing package %s; copying instead.' % source,
          file=sys.stderr)
    shutil.copy2(source, target)


def generate(env):
    if env['PLATFORM'] == 'darwin' or int(env.get('cross_osx', 0)):
        env.CBAddVariables(
            # put sign_* in scons_options.py (build.json)
            # if not sign_keychain, the default (login) keychain will be used
            # if not sign_id_installer, productsign will be skipped
            # sign_id_app is required for sign_apps and sign_tools
            # sign_prefix is required for sign_tools
            # global; cannot currently be overridden per-component
            BoolVariable('sign_disable', 'Disable codesign', 0),
            ('sign_keychain', 'Keychain that has signatures'),
            ('sign_id_installer', 'Installer signature name'),
            ('sign_id_app', 'Application/Tool signature name'),
            ('sign_prefix', 'codesign identifier prefix'),
            )

        env.AddMethod(UnlockKeychain)
        env.AddMethod(SignExecutable)
        env.AddMethod(SignApplication)
        env.AddMethod(SignPackage)
        env.AddMethod(SignOrCopyPackage)

    return True


def exists():
    return 1
