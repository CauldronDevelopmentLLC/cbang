from SCons.Script import *


def run_tests(env):
    import shlex
    import subprocess
    import sys

    sys.exit(subprocess.call(shlex.split(env.get('TEST_COMMAND'))))


def generate(env):
    import os

    cmd = 'python tests/testHarness -C tests --diff-failed --view-failed ' \
        '--view-unfiltered --save-failed --build'

    if 'DOCKBOT_MASTER_PORT' in os.environ: cmd += ' --no-color'

    env.CBAddVariables(('TEST_COMMAND', '`test` target command line', cmd))

    if 'test' in COMMAND_LINE_TARGETS: env.CBAddConfigureCB(run_tests)


def exists(): return 1
