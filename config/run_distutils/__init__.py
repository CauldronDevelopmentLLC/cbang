from SCons.Script import *
import os


def generate(env):
    env.SetDefault(RUN_DISTUTILS = 'python')
    env.SetDefault(RUN_DISTUTILSOPTS = 'build')

    if 'RUN_DISTUTILS' in os.environ:
        env['RUN_DISTUTILS'] = os.environ['RUN_DISTUTILS']
    if 'RUN_DISTUTILSOPTS' in os.environ:
        env['RUN_DISTUTILSOPTS'] = os.environ['RUN_DISTUTILSOPTS']

    bld = Builder(action = '$RUN_DISTUTILS $SOURCE $RUN_DISTUTILSOPTS')
    env.Append(BUILDERS = {'RunDistUtils' : bld})


def exists():
    return 1
