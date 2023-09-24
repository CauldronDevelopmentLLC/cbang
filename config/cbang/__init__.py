from SCons.Script import *
import inspect
import platform
import os


def GetHome():
    path = inspect.getfile(inspect.currentframe())
    return os.path.dirname(os.path.abspath(path))


def configure_deps(conf, local = True, with_openssl = True,
                   with_local_boost = True):
    env = conf.env

    conf.CBConfig('ZLib', not local)
    conf.CBConfig('bzip2', not local)
    conf.CBConfig('XML', not local)
    conf.CBConfig('sqlite3', not local)
    conf.CBConfig('libyaml', not local)
    conf.CBConfig('leveldb', False)

    env.AppendUnique(prefer_dynamic = ['mariadbclient'])
    if conf.CBCheckCHeader('mysql/mysql.h') and \
            (conf.CBCheckLib('mariadbclient') or
             conf.CBCheckLib('mysqlclient')) and \
            conf.CBCheckFunc('mysql_real_connect_start'):
        env.CBConfigDef('HAVE_MARIADB')
        env.cb_enabled.add('mariadb')

    # LZ4
    conf.CBCheckLib('lz4')

    # Boost
    if env['PLATFORM'] == 'win32': env.CBDefine('BOOST_ALL_NO_LIB')
    if not with_local_boost:
        conf.CBRequireLib('boost_filesystem')
        conf.CBRequireLib('boost_iostreams')
        conf.CBRequireLib('boost_regex')
        conf.CBRequireLib('boost_system')

    # clock_gettime() needed by boost iterprocess
    if env['PLATFORM'] == 'posix' and int(env.get('cross_osx', 0)) == 0 \
            and not conf.CBCheckFunc('clock_gettime'):
        conf.CBRequireLib('rt')
        conf.CBRequireFunc('clock_gettime')

    # Must be after 'rt'
    if conf.CBConfig('event', False): conf.CBConfig('re2', not local)

    # EPoll support
    if conf.CBCheckFunc('epoll_create1'): env.CBConfigDef('HAVE_EPOLL')

    if with_openssl: conf.CBConfig('openssl', False, version = '1.1.0')
    conf.CBConfig('v8', False)

    if env['PLATFORM'] == 'win32' or int(env.get('cross_mingw', 0)):
        if not conf.CBCheckLib('ws2_32'): conf.CBRequireLib('wsock32')
        conf.CBCheckLib('winmm')
        conf.CBRequireLib('setupapi')

    else: conf.CBConfig('pthreads')

    # OSX frameworks
    if env['PLATFORM'] == 'darwin' or int(env.get('cross_osx', 0)):
        if not (conf.CheckOSXFramework('CoreServices') and
                conf.CheckOSXFramework('IOKit') and
                conf.CheckOSXFramework('Security') and
                conf.CheckOSXFramework('CoreFoundation')):
            raise SCons.Errors.StopError('Need CoreServices, IOKit, Security '
                                         '& CoreFoundation frameworks')

    conf.CBConfig('valgrind', False)

    # Debug
    if env.get('debug', 0):
        if conf.CBCheckCHeader('execinfo.h') and conf.CBCheckCHeader('bfd.h') \
           and conf.CBCheckLib('bfd'):
            conf.CBCheckLib('iberty')
            conf.CBCheckLib('sframe')
            conf.CBCheckLib('zstd')
            env.CBConfigDef('HAVE_CBANG_BACKTRACE')

            # Check bfd for API change
            if not conf.CBCheckFunc('bfd_get_error_handler'):
                env.CBConfigDef('HAVE_BFD_ERROR_HANDLER_VPRINTFLIKE')

        elif env.get('backtrace_debugger', 0):
            raise SCons.Errors.StopError(
                'execinfo.h, bfd.h and libbfd needed for backtrace_debuger')

        env.CBDefine('CBANG_DEBUG_LEVEL=' + str(env.get('debug_level', 1)))


def configure(conf):
    env = conf.env

    home = GetHome() + '/../..'
    env.AppendUnique(CPPPATH = [home + '/src', home + '/include',
                                home + '/src/boost'])
    env.AppendUnique(LIBPATH = [home + '/lib'])

    with open(home + '/include/cbang/config.h', 'r') as f:
        config = f.read()
        with_openssl = config.find('#define HAVE_OPENSSL') != -1
        with_local_boost = config.find('#define HAVE_LOCAL_BOOST') != -1

    if not env.CBConfigEnabled('cbang-deps'):
        conf.CBConfig('cbang-deps', local = False, with_openssl = with_openssl,
                      with_local_boost = with_local_boost)

    if with_local_boost: conf.CBRequireLib('cbang-boost')

    conf.CBRequireLib('cbang')
    if platform.system() == 'FreeBSD': conf.CBRequireLib('sysinfo')
    conf.CBRequireCXXHeader('cbang/Exception.h')
    env.CBDefine('HAVE_CBANG')


def generate(env):
    env.CBAddConfigTest('cbang', configure)
    env.CBAddConfigTest('cbang-deps', configure_deps)

    env.CBAddVariables(
        BoolVariable('backtrace_debugger', 'Enable backtrace debugger', 0),
        ('debug_level', 'Set log debug level', 1))

    env.CBLoadTools('''sqlite3 openssl pthreads valgrind osx ZLib bzip2
        XML v8 event re2 libyaml leveldb'''.split(), GetHome() + '/..')


def exists(env):
    return 1
