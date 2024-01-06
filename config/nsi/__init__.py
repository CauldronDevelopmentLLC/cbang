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
from SCons.Action import CommandAction


deps = ['find_dlls']


def build_function(target, source, env):
    nsi = str(source[0])

    install_files = env.get('nsis_install_files', [])

    # Find DLLs
    if 'nsi_dll_deps' in env:
        for path in env.FindDLLs(env.get('nsi_dll_deps')):
            install_files += [path]

    # Install files
    files = '\n'
    for path in install_files:
        if os.path.isdir(path):
            files += '  SetOutPath "$INSTDIR\\%s"\n' % os.path.basename(path)
            files += '  File /r "%s\\*.*"\n' % path
            files += '  SetOutPath "$INSTDIR"\n'

        else: files += '  File "%s"\n' % path

    env['NSIS_INSTALL_FILES'] = files

    # Set package name
    env.Replace(package = str(target[0]))

    if not 'PACKAGE_ARCH' in env:
        import platform
        env['PACKAGE_ARCH'] = platform.architecture()[0][:2]

    tmp = nsi + '.tmp'
    with open(nsi, 'r') as input:
        with open(tmp, 'w') as output:
            output.write(input.read() % env)

    action = CommandAction('$NSISCOM $NSISOPTS ' + tmp)
    ret = action.execute(target, [tmp], env)
    if ret != 0: return ret

    if env.get('code_sign_key', None):
        cmd = '"$SIGNTOOL" sign /f "%s"' % env.get('code_sign_key')
        if 'code_sign_key_pass' in env:
            cmd += ' /p "%s"' % env.get('code_sign_key_pass')
        if 'summary' in env: cmd += ' /d "%s"' % env.get('summary')
        if 'url' in env: cmd += ' /du "%s"' % env.get('url')
        if 'timestamp_url' in env: cmd += ' /t "%s"' % env.get('timestamp_url')
        cmd += ' $TARGET'

        action = CommandAction(cmd)
        return action.execute(target, [], env)

    return 0


def generate(env):
    env.CBLoadTool('find_dlls')

    env.SetDefault(NSISCOM = 'makensis', NSISOPTS = '', SIGNTOOL = 'signtool')
    bld = Builder(action = build_function)
    env.Append(BUILDERS = {'Nsis' : bld})


def exists():
    return 1
