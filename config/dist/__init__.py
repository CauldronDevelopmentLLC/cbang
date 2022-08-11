import os
import sys
import tarfile
import re
import platform
import time
from SCons.Script import *


def find_files(path, exclude = None):
    if not os.path.exists(path): return []
    dir, filename = os.path.split(path)

    if exclude and exclude.match(filename): return []
    if not os.path.isdir(path): return [(path, filename)]

    files = []
    for f in os.listdir(path):
        children = find_files(path + '/' + f, exclude)
        files += [(path, filename + '/' + name) for path, name in children]

    return files


def modify_targets(target, source, env):
    if env.get('debug', False): mode = 'debug'
    else: mode = 'release'
    mode = env.get('PACKAGE_MODE', mode)

    dist_ver = env.get('dist_version', env.subst('$PACKAGE_VERSION'))

    vars = {
        'machine': platform.machine(),
        'bits':    platform.architecture()[0],
        'system':  platform.system(),
        'release': platform.release(),
        'version': dist_ver,
        'date':    time.strftime('%Y%m%d'),
        'mode':    mode,
        }

    build = env.get('dist_build')
    if len(build) and build[0] not in '-_': build = '_' + build
    if len(dist_ver): build = '_' + dist_ver + build

    target = ((str(target[0]) + build) % vars % env._dict) + '.tar'

    return [target.lower(), source]


def modify_targets_bz2(target, source, env):
    target, source = modify_targets(target, source, env)
    return [target + '.bz2', source]


def build_function(target, source, env):
    target = str(target[0])

    # Write 'dist.txt'
    with open('dist.txt', 'w') as f: f.write(target)

    m = re.match(r'^(.*)\.tar(\.\w+)?$', target)
    dist_name = m.group(1)

    mode = 'w:bz2' if target.endswith('.bz2') else 'w'

    with tarfile.open(target, mode = mode) as tar:
        exclude_pats = env.get('DIST_EXCLUDES')
        exclude_re = re.compile('^((' + ')|('.join(exclude_pats) + '))$')

        for src in map(str, source):
            for path, name in find_files(src, exclude_re):
                tar.add(path, dist_name + '/' + name)


def generate(env):
    env.SetDefault(DIST_EXCLUDES = [r'\.svn', r'\.sconsign.dblite', r'.*\.obj',
                                    r'\.sconf_temp', r'.*~', r'.*\.o'])

    env.CBAddVariables(
            ('dist_version', 'Set dist file version', None),
            ('dist_build', 'Set dist file build info', '-%(bits)s-%(mode)s'))

    bld = Builder(action = build_function,
                  source_factory = SCons.Node.FS.Entry,
                  source_scanner = SCons.Defaults.DirScanner,
                  emitter = modify_targets)
    bld_bz2 = Builder(action = build_function,
                      source_factory = SCons.Node.FS.Entry,
                      source_scanner = SCons.Defaults.DirScanner,
                      emitter = modify_targets_bz2)

    env.Append(BUILDERS = {'TarBZ2Dist' : bld_bz2})
    env.Append(BUILDERS = {'TarDist' : bld})

    return True


def exists():
    return 1
