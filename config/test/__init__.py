from SCons.Script import *
import shlex


def run_tests(env):
    import shlex
    import subprocess
    import sys

    cmd = shlex.split(env.get('TEST_COMMAND'))
    print('Executing:', cmd)
    sys.exit(subprocess.call(cmd))


def generate(env):
    import os
    import sys

    python = sys.executable

    dir = os.path.dirname(os.path.realpath(__file__))
    testHarness = dir + '/../../tests/testHarness'

    if env['PLATFORM'] == 'win32':
        python = python.replace('\\', '\\\\')
        testHarness = testHarness.replace('\\', '\\\\')

    cmd = [python, testHarness, '-C', 'tests', '--diff-failed', '--view-failed',
           '--view-unfiltered', '--save-failed', '--build']
    cmd = ' '.join([shlex.quote(s) for s in cmd]) # shlex.join() not in < 3.8

    env.CBAddVariables(('TEST_COMMAND', '`test` target command line', cmd))

    if 'test' in COMMAND_LINE_TARGETS: env.CBAddConfigureCB(run_tests)


def exists(): return 1
