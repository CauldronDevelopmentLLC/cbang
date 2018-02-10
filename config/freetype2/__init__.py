import os
from SCons.Script import *


def configure(conf):
    env = conf.env

    conf.CBCheckHome('freetype2',
                     inc_suffix=['/include', '/include/freetype2'])

    if not 'FREETYPE2_INCLUDE' in os.environ:
        try:
            env.ParseConfig('freetype-config --cflags')
        except OSError: pass

    if env['PLATFORM'] == 'darwin' or int(env.get('cross_osx', 0)):
        if not conf.CheckOSXFramework('CoreServices'):
            raise Exception('Need CoreServices framework')

    conf.CBRequireCHeader('ft2build.h')
    conf.CBRequireLib('freetype')
    conf.CBConfig('zlib')
    conf.CBCheckLib('png')

    return True


def generate(env):
    env.CBAddConfigTest('freetype2', configure)
    env.CBLoadTools('osx zlib')


def exists():
    return 1
