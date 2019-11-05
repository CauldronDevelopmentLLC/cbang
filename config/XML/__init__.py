import sys
from SCons.Script import *


def configure(conf): return conf.CBConfig('expat')


def generate(env):
    env.CBAddConfigTest('XML', configure)
    env.CBLoadTool('expat')


def exists(): return 1
