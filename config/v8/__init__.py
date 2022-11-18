from SCons.Script import *
import platform


def configure(conf):
    lib_suffix = ['/lib']
    if conf.env.get('debug', False): lib_suffix.append('/build/Release/lib')
    else: lib_suffix.append('/build/Debug/lib')

    if not 'V8_INCLUDE' in os.environ and not 'V8_HOME' in os.environ:
        os.environ['V8_INCLUDE'] = '/usr/include/v8'

    conf.CBCheckHome('v8', lib_suffix = lib_suffix)

    if conf.env['PLATFORM'] == 'win32' or int(conf.env.get('cross_mingw', 0)):
        conf.CBRequireLib('winmm')

    conf.CBRequireCXXHeader('v8.h')
    conf.CBRequireCXXHeader('libplatform/libplatform.h')

    if conf.CBCheckLib('v8_monolith'): pass
    elif conf.CBCheckLib('v8'): conf.CBCheckLib('v8_libplatform')
    else:
        conf.CBRequireLib('v8_snapshot')

        if not conf.CBCheckLib('v8_base'):
            if platform.architecture()[0] == '64bit':
                conf.CBRequireLib('v8_base.x64')
            else: conf.CBRequireLib('v8_base.ia32')

    if conf.env.get('v8_compress_pointers'):
        conf.env.CBConfigDef('V8_COMPRESS_POINTERS')

    conf.env.CBConfigDef('HAVE_V8')


def generate(env):
    env.CBAddConfigTest('v8', configure)


def exists():
    return 1
