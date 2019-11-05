from __future__ import print_function

from SCons.Script import *
import inspect
import traceback


def try_config(conf, command):
    env = conf.env

    try:
        env.ParseConfig(command)
        env.Prepend(LIBS = ['util', 'm', 'dl', 'z'])

    except OSError:
        return False

    if conf.CheckHeader('Python.h') and conf.CheckFunc('Py_Initialize'):
        env.ParseConfig(command)
        env.CBConfigDef('HAVE_PYTHON');
        env.Prepend(LIBS = ['util', 'm', 'dl', 'z'])

        return True

    return False


def configure(conf):
    env = conf.env

    if not env.get('python', 0): return False

    python_version = env.get('python_version', '')

    dir = os.path.dirname(inspect.getfile(inspect.currentframe()))
    cmd = "python%s '%s'/python-config.py" % (python_version, dir)

    return try_config(conf, cmd)


def generate(env):
    env.CBAddConfigTest('python', configure)

    env.CBAddVariables(
        BoolVariable('python', 'Set to 0 to disable python', 1),
        ('python_version', 'Set python version', ''))



def exists():
    return 1
