from SCons.Script import *


def configure(conf):
    conf.CBCheckHome('libyaml', lib_suffix = ['/src'],
                     inc_suffix = ['/include'])
    conf.CBRequireHeader('yaml.h')
    conf.CBRequireLib('yaml')


def generate(env):
    env.CBAddConfigTest('libyaml', configure)


def exists():
    return 1
