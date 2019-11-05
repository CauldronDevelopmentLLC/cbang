from SCons.Script import *


def configure(conf):
    conf.CBCheckHome('leveldb')
    conf.CBRequireLib('snappy')
    conf.CBRequireLib('leveldb')
    conf.CBRequireCXXHeader('leveldb/db.h')
    conf.env.CBConfigDef('HAVE_LEVELDB')


def generate(env): env.CBAddConfigTest('leveldb', configure)


def exists(): return 1
