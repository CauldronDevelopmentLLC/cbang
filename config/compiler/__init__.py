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

import copy
import re
import os
import sys
from platform import machine, architecture
from SCons.Script import *
from subprocess import *
from SCons.Util import MD5signature
import SCons.Action
import SCons.Builder
import SCons.Tool


def gcc_version_str(env):
    import shlex
    cmd = shlex.split(env.get('CC', 'gcc')) + ['-dumpversion']
    return check_output(cmd).strip().decode('utf8')


def gcc_version(env):
    return tuple(map(int, gcc_version_str(env).split('.')))



def CheckRDynamic(context):
    context.Message('Checking for -rdynamic...')
    env = context.env
    flags = env['LINKFLAGS']
    context.env.AppendUnique(LINKFLAGS = ['-rdynamic'])
    result = context.TryLink('int main() {return 0;}\n', '.c')
    context.Result(result)
    env['LINKFLAGS'] = flags
    return result


def configure(conf, cstd = 'c99'):
    env = conf.env
    if env.GetOption('clean'): return

    # Get options
    cc            =     env.get('cc',     '').strip()
    cxx           =     env.get('cxx',    '').strip()
    ranlib        =     env.get('ranlib', '').strip()
    strip         =     env.get('strip',  '').strip()
    debug         = int(env.get('debug'))
    optimize      = int(env.get('optimize'))
    if optimize == -1: optimize = not debug
    globalopt     = int(env.get('globalopt'))
    mach          =     env.get('mach')
    strict        = int(env.get('strict'))
    threaded      = int(env.get('threaded'))
    profile       = int(env.get('profile'))
    tcmalloc      = int(env.get('tcmalloc'))
    gperf         = int(env.get('gperf'))
    depends       = int(env.get('depends'))
    compiler      =     env.get('compiler')
    distcc        = int(env.get('distcc'))
    ccache        = int(env.get('ccache'))
    ccflags       =     env.get('ccflags')
    cxxflags      =     env.get('cxxflags')
    linkflags     =     env.get('linkflags')
    cxxstd        =     env.get('cxxstd')
    dbgstdcxx     =     env.get('dbgstdcxx')
    platform      =     env.get('platform')
    static        = int(env.get('static'))
    num_jobs      =     env.get('num_jobs')
    osx_min_ver   =     env.get('osx_min_ver')
    osx_sdk_root  =     env.get('osx_sdk_root')
    osx_archs     =     env.get('osx_archs')
    win32_thread  =     env.get('win32_thread')

    if platform != '': env.Replace(PLATFORM = platform)

    # Select compiler
    compiler_mode = None

    if compiler:
        if compiler == 'gnu':
            Tool('gcc')(env)
            Tool('g++')(env)
            compiler_mode = 'gnu'

        elif compiler == 'clang':
            env.Replace(CC = 'clang')
            env.Replace(CXX = 'clang++')
            compiler_mode = 'gnu'

        elif compiler == 'posix':
            Tool('cc')(env)
            Tool('cxx')(env)
            Tool('link')(env)
            Tool('ar')(env)
            Tool('as')(env)
            compiler_mode = 'unknown'

        elif compiler != 'default':
            Tool(compiler)(env)


    if compiler_mode is None:
        if env['CC'] == 'cl' or env['CC'] == 'icl': compiler_mode = 'msvc'
        elif env['CC'] == 'gcc' or env['CC'] == 'icc': compiler_mode = 'gnu'
        else: compiler_mode = 'unknown'

    if compiler == 'default':
        _cc = env['CC']
        if _cc == 'cl': compiler = 'msvc'
        elif _cc == 'gcc': compiler = 'gnu'

    if cc: env.Replace(CC = cc)
    if cxx: env.Replace(CXX = cxx)
    if ranlib: env.Replace(RANLIB = ranlib)
    if strip: env.Replace(STRIP = strip)

    env.__setitem__('compiler', compiler)
    env.__setitem__('compiler_mode', compiler_mode)

    print('   Compiler:', env['CC'], '(%s)' % compiler)
    print('   Platform:', env['PLATFORM'])
    print('       Mode:', compiler_mode)
    print('       Arch:', env['TARGET_ARCH'])

    if compiler == 'gnu': print('GCC Version:', gcc_version_str(env))


    # SCONS_JOBS environment variable
    if num_jobs < 1 and 'SCONS_JOBS' in os.environ:
        num_jobs = int(os.environ.get('SCONS_JOBS', num_jobs))

    # Default num jobs
    if num_jobs < 1:
        import multiprocessing
        num_jobs = multiprocessing.cpu_count()

    SetOption('num_jobs', num_jobs)
    print('       Jobs:', GetOption('num_jobs'))


    # distcc
    if distcc and compiler == 'gnu':
        env.Replace(CC = 'distcc ' + env['CC'])
        env.Replace(CXX = 'distcc ' + env['CXX'])

    # cccache
    if ccache and compiler == 'gnu':
        env.Replace(CC = 'ccache ' + env['CC'])
        env.Replace(CXX = 'ccache ' + env['CXX'])


    # Exceptions
    if compiler_mode == 'msvc':
        env.AppendUnique(CCFLAGS = ['/EHa']) # Asynchronous


    # Disable troublesome warnings
    if compiler_mode == 'msvc':
        env.CBDefine('_CRT_SECURE_NO_WARNINGS')

        for warning in [4297, 4103]:
            env.AppendUnique(CCFLAGS = ['/wd%d' % warning])


    # Profiler flags
    if profile:
        if compiler_mode == 'gnu':
            env.AppendUnique(CCFLAGS = ['-pg'])
            env.AppendUnique(LINKFLAGS = ['-pg'])


    # tcmalloc & gperf
    if tcmalloc and compiler_mode == 'gnu':
        env.AppendUnique(CCFLAGS =
                         ['-fno-builtin-malloc', '-fno-builtin-calloc',
                          '-fno-builtin-realloc', '-fno-builtin-free'])

    if tcmalloc and gperf:
        env.AppendUnique(prefer_dynamic = ['tcmalloc_and_profiler'])
        conf.CBRequireLib('tcmalloc_and_profiler')
    elif tcmalloc:
        env.AppendUnique(prefer_dynamic = ['tcmalloc'])
        conf.CBRequireLib('tcmalloc')
    elif gperf:
        env.AppendUnique(prefer_dynamic = ['profiler'])
        conf.CBRequireLib('profiler')


    # Debug flags
    if debug:
        if compiler_mode == 'msvc':
            env.AppendUnique(CCFLAGS = ['/W1'])
            env['PDB'] = '${TARGET}.pdb'
            env.AppendUnique(LINKFLAGS = ['/DEBUG', '/MAP:${TARGET}.map'])

        elif compiler_mode == 'gnu':
            if compiler == 'gnu':
                env.AppendUnique(CCFLAGS = ['-g', '-Wall'])
                if conf.CheckRDynamic():
                    env.AppendUnique(LINKFLAGS = ['-rdynamic']) # for backtrace
            else: # clang and others
                env.AppendUnique(CCFLAGS = ['-g', '-Wall'])

            if strict: env.AppendUnique(CCFLAGS = ['-Werror'])

        env.CBDefine('DEBUG')

    else: # !debug
        if compiler_mode == 'gnu': # Strip symbols
            if compiler != 'clang': env.AppendUnique(LINKFLAGS = ['-Wl,-s'])
            env.AppendUnique(LINKFLAGS = ['-Wl,-x'])

        if compiler == 'gnu': # Enable dead code removal
            if env['PLATFORM'] != 'darwin':
                env.AppendUnique(LINKFLAGS = ['-Wl,--gc-sections'])
            env.AppendUnique(
                CCFLAGS = ['-ffunction-sections', '-fdata-sections'])

        env.CBDefine('NDEBUG')


    # Optimizations
    if optimize:
        if compiler_mode == 'gnu':
            env.AppendUnique(CCFLAGS = ['-O3', '-funroll-loops'])

        elif compiler_mode == 'msvc':
            env.AppendUnique(CCFLAGS = ['/O2', '/Zc:throwingNew'])

        # Whole program optimizations
        if globalopt:
            if compiler == 'msvc':
                env.AppendUnique(CCFLAGS = ['/GL'])
                env.AppendUnique(LINKFLAGS = ['/LTCG'])
                env.AppendUnique(ARFLAGS = ['/LTCG'])

        # Instruction set optimizations
        if compiler == 'msvc':
            if mach: env.AppendUnique(CCFLAGS = ['/arch:' + mach.upper()])

        elif mach: env.AppendUnique(CCFLAGS = ['-m' + mach.lower()])


    # Alignment
    if compiler_mode == 'gnu' and (7,) <= gcc_version(env):
        if not (compiler == 'clang' and env['PLATFORM'] == 'darwin'):
            env.AppendUnique(CXXFLAGS = ['-faligned-new'])

    # Dependency files
    if depends and compiler_mode == 'gnu':
        env.AppendUnique(CCFLAGS = ['-MMD -MF ${TARGET}.d'])

    # No PIE with GCC 6+
    if compiler_mode == 'gnu' and (5,) <= gcc_version(env):
        env.AppendUnique(CCFLAGS = '-fno-pie')
        if compiler != 'clang': env.AppendUnique(LINKFLAGS = '-no-pie')


    # C mode
    if cstd and compiler_mode == 'gnu':
        env.AppendUnique(CFLAGS = ['-std=' + cstd])

    # C++ mode
    if cxxstd:
        if compiler_mode == 'gnu':
            env.AppendUnique(CXXFLAGS = ['-std=' + cxxstd])
        if compiler_mode == 'msvc':
            if cxxstd in ('c++14', 'c++17', 'c++20'):
                env.AppendUnique(CXXFLAGS = ['/std:' + cxxstd])

    # GNU libstdc++ debugging
    if dbgstdcxx and compiler_mode == 'gnu':
        env.CBDefine('_GLIBCXX_DEBUG')
        env.CBDefine('_GLIBCXX_DEBUG_BACKTRACE')
        conf.CBRequireLib('stdc++_libbacktrace')


    # Threads
    if threaded:
        if compiler_mode == 'gnu':
            conf.CBRequireLib('pthread')

            env.AppendUnique(LINKFLAGS = ['-pthread'])
            env.CBDefine('_REENTRANT')

        elif compiler_mode == 'msvc':
            if win32_thread == 'static':
                if debug: env.AppendUnique(CCFLAGS = ['/MTd'])
                else: env.AppendUnique(CCFLAGS = ['/MT'])
            else:
                if debug: env.AppendUnique(CCFLAGS = ['/MDd'])
                else: env.AppendUnique(CCFLAGS = ['/MD'])


    # Link flags
    if compiler_mode == 'msvc' and not optimize:
        env.AppendUnique(LINKFLAGS = ['/INCREMENTAL:NO'])

    if compiler_mode == 'msvc':
        env.Append(LINKFLAGS = ['/subsystem:${subsystem},${subsystem_version}'])

    # static
    if static:
        if compiler_mode == 'gnu' and env['PLATFORM'] != 'darwin':
            env.AppendUnique(LINKFLAGS = ['-static'])

    elif env.get('mostly_static', False) and compiler == 'gnu':
        env.AppendUnique(LINKFLAGS = ['-static-libstdc++', '-static-libgcc'])


    # Don't link unneeded dynamic libs
    if compiler == 'gnu' and env['PLATFORM'] != 'darwin':
        env.PrependUnique(LINKFLAGS = ['-Wl,--as-needed'])

    # For windows
    if compiler_mode == 'msvc': env.CBDefine('NOMINMAX')

    # For darwin
    if env['PLATFORM'] == 'darwin':
        env.CBDefine('__APPLE__')

        if osx_archs and compiler_mode == 'gnu':
            # note: only apple compilers support multipe -arch options
            for arch in osx_archs.split():
                env.Append(CCFLAGS = ['-arch', arch])
                env.Append(LINKFLAGS = ['-arch', arch])

        if osx_min_ver:
            flag = '-mmacosx-version-min=' + osx_min_ver
            env.AppendUnique(LINKFLAGS = [flag])
            env.AppendUnique(CCFLAGS = [flag])

        if osx_sdk_root:
            env.Append(CCFLAGS = ['-isysroot', osx_sdk_root])
            if compiler == 'gnu':
                env.Append(LINKFLAGS = ['-isysroot', osx_sdk_root])

        env.Replace(ARCOM = 'rm -f $TARGET; $AR $ARFLAGS $TARGET $SOURCES')

    # Both gcc and clang treat signedness of char depending on target platform.
    # For Intel x86 and x86_64, it's a signed type by default.
    # For ARM (including AArch64), it's an unsigned type by default.
    # There are several places in the code, where 'char' implicitly assumed to
    # be a signed type. The simplest way to avoid problems in such a case is to
    # ask the compiler to treat the 'char" as a signed type regardless of target
    # platform.
    if compiler_mode == 'gnu':
        env.Append(CFLAGS   = ['-fsigned-char'])
        env.Append(CXXFLAGS = ['-fsigned-char'])

    # User flags (Must be last)
    if ccflags: env.Append(CCFLAGS = ccflags.split())
    if cxxflags: env.Append(CXXFLAGS = cxxflags.split())
    if linkflags: env.Append(LINKFLAGS = linkflags.split())


def get_lib_path_env(env):
    eenv = copy.copy(os.environ)

    path = []

    if 'LIBPATH' in env: path += list(env['LIBPATH'])
    if 'LIBRARY_PATH' in eenv:
        path += eenv['LIBRARY_PATH'].split(':')

    eenv['LIBRARY_PATH'] = ':'.join(path)

    return eenv


def FindLibPath(env, lib):
    if env.get('compiler_mode', '') != 'gnu': return

    if lib.startswith(os.sep) or lib.endswith(env['LIBSUFFIX']):
        return lib # Already a path

    eenv = get_lib_path_env(env)
    cmd = env['CXX'].split()
    libpat = env['LIBPREFIX'] + lib + env['LIBSUFFIX']

    path = Popen(cmd + ['-print-file-name=' + libpat],
                 stdout = PIPE, env = eenv).communicate()[0].strip()
    if isinstance(path, bytes): path = path.decode()

    if path != libpat: return path


def build_pattern(env, name):
    pats = env.get(name)
    if hasattr(pats, 'split'): pats = pats.split()
    pats += env[name.upper()]

    return env.CBBuildSetRegex(pats)


def prefer_static_libs(env):
    if env.get('compiler_mode', '') != 'gnu': return

    mostly_static = env.get('mostly_static')
    prefer_static = build_pattern(env, 'prefer_static')
    prefer_dynamic = build_pattern(env, 'prefer_dynamic')
    require_static = build_pattern(env, 'require_static')

    libs = []
    changed = False

    for lib in env['LIBS']:
        name = str(lib)

        if require_static.match(name) or prefer_static.match(name) or \
                (mostly_static and not prefer_dynamic.match(name)):
            path = FindLibPath(env, name)
            if path is not None:
                changed = True
                libs.append(File(path))
                continue

        if require_static.match(name):
            raise Exception('Failed to find static library for "%s"' % name)

        libs.append(lib)

    if changed:
        env.Replace(LIBS = libs)

        # Force two pass link to resolve circular dependencies
        if env['PLATFORM'] == 'posix':
            env['_LIBFLAGS'] = \
                '-Wl,--start-group ' + env['_LIBFLAGS'] + ' -Wl,--end-group'


def CBConfConsole(env, entry = 'mainCRTStartup'):
    if env['PLATFORM'] == 'win32':
        env.Replace(subsystem = 'windows')

        if env.get('compiler_mode') == 'msvc':
            env.Append(LINKFLAGS = ['/entry:' + entry])


def generate(env):
    env.CBAddConfigTest('compiler', configure)
    env.CBAddTest(CheckRDynamic)
    env.CBAddConfigFinishCB(prefer_static_libs)
    env.AddMethod(CBConfConsole)

    env.SetDefault(PREFER_DYNAMIC = 'pthread dl rt'.split())
    env.SetDefault(PREFER_STATIC = [])
    env.SetDefault(REQUIRE_STATIC = [])

    env.CBAddVariables(
        ('cc', 'Set C compiler executable', ''),
        ('cxx', 'Set C++ compiler executable', ''),
        ('ranlib', 'Set ranlib executable', ''),
        ('strip', 'Set strip executable', ''),
        ('optimize', 'Enable or disable optimizations', -1),
        ('globalopt', 'Enable or disable global optimizations', 0),
        ('mach', 'Set machine instruction set', ''),
        BoolVariable('debug', 'Enable or disable debug options',
                     os.getenv('DEBUG_MODE', 0)),
        BoolVariable('strict', 'Enable or disable strict options', 1),
        BoolVariable('threaded', 'Enable or disable thread support', 1),
        BoolVariable('profile', 'Enable or disable profiler', 0),
        BoolVariable('tcmalloc', 'Enable or disable tcmalloc', 0),
        BoolVariable('gperf', 'Enable or disable gperf', 0),
        BoolVariable('depends', 'Enable or disable dependency files', 0),
        BoolVariable('distcc', 'Enable or disable distributed builds', 0),
        BoolVariable('ccache', 'Enable or disable cached builds', 0),
        EnumVariable('platform', 'Override default platform', '',
                   allowed_values = ('', 'win32', 'posix', 'darwin')),
        ('ccflags', 'Set extra C and C++ compiler flags', None),
        ('cxxflags', 'Set extra C++ compiler flags', None),
        ('linkflags', 'Set extra linker flags', None),
        EnumVariable(
            'cxxstd', 'Set C++ language standard', 'c++14',
            allowed_values = ('c++98', 'c++11', 'c++14', 'c++17', 'c++20')),
        BoolVariable('dbgstdcxx', 'Enable GNU libstdc++ debugging', 0),
        EnumVariable(
            'compiler', 'Select compiler', 'default',
            allowed_values = ('default', 'gnu', 'msvc', 'posix', 'clang')),
        BoolVariable('static', 'Link to static libraries', 0),
        BoolVariable('mostly_static', 'Prefer static libraries', 0),
        ('prefer_static', 'Libraries where the static version is prefered', ''),
        ('require_static', 'Libraries which must be linked statically', ''),
        ('prefer_dynamic', 'Libraries where the dynamic version is prefered, ' +
         'regardless of "mostly_static"', ''),
        ('num_jobs', 'Set the concurrency level.', -1),
        ('osx_min_ver', 'Set minimum supported OSX version.', None),
        ('osx_sdk_root', 'Set OSX SDK root.', None),
        ('osx_archs', 'Set OSX gcc target architectures.', None),
        EnumVariable(
            'win32_thread', 'Windows thread mode.', 'static',
            allowed_values = ('static', 'dynamic')),
        EnumVariable(
            'subsystem', 'Windows subsystem', 'console',
            allowed_values = ('windows', 'console', 'posix', 'native')),
        ('subsystem_version', 'Windows subsystem version', 6)
        )


def exists():
    return 1
