from SCons.Script import *


def configure(conf):
    env = conf.env

    conf.CBCheckHome('expat')

    conf.CBRequireCHeader('expat.h')

    if env['PLATFORM'] == 'win32':
        if not conf.CBCheckLib('libexpatMT'):
            conf.CBRequireLib('expat')
        env.CBDefine('XML_STATIC')

    else: conf.CBRequireLib('expat')


def generate(env):
    env.CBAddConfigTest('expat', configure)


def exists():
    return 1
