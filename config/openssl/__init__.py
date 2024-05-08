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

import os
from SCons.Script import *


def check_version(context, version):
    context.Message("Checking for openssl version >= %s..." % version)

    version = version.split('.')
    version += [0] * (3 - len(version))

    src = '''
      #include <openssl/opensslv.h>
      int main() {
        const unsigned v = (%s << 28) + (%s << 20) + (%s << 12);
        int a[1 - 2 * (OPENSSL_VERSION_NUMBER < v)];
        a[0] = 0;
        return a[0];
      }
    ''' % tuple(version)

    ret = context.TryCompile(src, '.cpp')

    context.Result(ret)
    return ret


def configure(conf, version = None):
    env = conf.env

    conf.CBCheckHome('openssl', inc_suffix = ['/inc32', '/include'],
                     lib_suffix = ['/out32', '/lib', ''])

    if env['PLATFORM'] == 'posix': conf.CBCheckLib('dl')

    if (conf.CBCheckCHeader('openssl/ssl.h') and
        (conf.CBCheckLib('crypto') and
         conf.CBCheckLib('ssl')) or
        (conf.CBCheckLib('libcrypto') and
         conf.CBCheckLib('libssl')) or
        (conf.CBCheckLib('libeay32MT') and
         conf.CBCheckLib('ssleay32MT')) or
        (conf.CBCheckLib('libeay32') and
         conf.CBCheckLib('ssleay32'))):

        if env['PLATFORM'] == 'win32' or int(env.get('cross_mingw', 0)):
            if not conf.CBCheckLib('ws2_32'): conf.CBRequireLib('wsock32')
            for lib in ['advapi32', 'gdi32', 'user32', 'crypt32']:
                conf.CBRequireLib(lib)

        if version is not None:
            if not conf.OpenSSLVersion(version):
                raise Exception('Insufficient OpenSSL version')

        env.CBConfigDef('HAVE_OPENSSL')
        return True

    else: raise Exception('Need openssl')

    return True


def generate(env):
    env.CBAddConfigTest('openssl', configure)
    env.CBAddTest('OpenSSLVersion', check_version)


def exists():
    return 1
