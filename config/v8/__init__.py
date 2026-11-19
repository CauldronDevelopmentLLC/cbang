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

from SCons.Script import *
import platform


v8_init_src = '''
  %s
  #include <v8.h>
  #include <libplatform/libplatform.h>
  #include <cstdio>
  #include <cstdlib>
  #include <csignal>

  static void on_abort(int) {std::_Exit(1);}

  int main() {
    // Fail quietly on V8's mismatch abort
  #ifdef _WIN32
    freopen("NUL", "w", stderr);
  #else
    freopen("/dev/null", "w", stderr);
  #endif
    signal(SIGABRT, on_abort);

    auto platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    return !v8::V8::Initialize();
  }
'''


def check_compress_pointers(context):
    # V8 aborts in Initialize() if the embedder's V8_COMPRESS_POINTERS does
    # not match the library's build, so ask the library which one it accepts.
    context.Message('Checking if libv8 uses pointer compression...')

    if context.TryRun(v8_init_src % '#define V8_COMPRESS_POINTERS', '.cpp')[0]:
        ret = True
    elif context.TryRun(v8_init_src % '', '.cpp')[0]: ret = False
    else: ret = platform.machine().endswith('64') # Cannot run, e.g. cross

    context.Result(ret)
    return ret


def configure(conf):
    lib_suffix = ['/lib']
    if conf.env.get('debug', False): lib_suffix.append('/build/Release/lib')
    else: lib_suffix.append('/build/Debug/lib')

    if not 'V8_INCLUDE' in os.environ and not 'V8_HOME' in os.environ:
        os.environ['V8_INCLUDE'] = '/usr/include/v8'

    conf.CBCheckHome('v8', lib_suffix = lib_suffix)

    if conf.env['PLATFORM'] == 'win32' or int(conf.env.get('cross_mingw', 0)):
        conf.CBRequireLib('winmm')
        conf.CBCheckLib('dbghelp')

    conf.CBRequireCXXHeader('v8.h')
    conf.CBRequireCXXHeader('libplatform/libplatform.h')

    if conf.CBCheckLib('v8_monolith'): pass
    elif conf.CBCheckLib('v8'): conf.CBCheckLib('v8_libplatform')
    else:
        conf.CBRequireLib('v8_snapshot')

        if not conf.CBCheckLib('v8_base'):
            if platform.architecture()[0] == '64bit':
                conf.CBRequireLib('v8_base.x64')
            else: conf.CBRequireLib('v8_base.ia32')

    compress = conf.env.get('v8_compress_pointers')
    if compress is None: compress = conf.V8CompressPointers()
    if compress:
        conf.env.CBConfigDef('V8_COMPRESS_POINTERS')

    conf.env.CBConfigDef('HAVE_V8')


def generate(env):
    env.CBAddConfigTest('v8', configure)
    env.CBAddTest('V8CompressPointers', check_compress_pointers)


def exists():
    return 1
