''' codesign '''

from SCons.Script import *

''' TODO
It is useful to have codesign/productsign apart from packaging.
So, port codesign/productsign from flatdistpkg to here.
Add apropriate methods for public use.
'''


def generate(env):
    if env['PLATFORM'] == 'darwin' or int(env.get('cross_osx', 0)):
        env.CBAddVariables(
            # put sign_* in scons_options.py (build.json)
            # if not sign_keychain, the default (login) keychain will be used
            # if not sign_id_installer, productsign will be skipped
            # sign_id_app is required for sign_apps and sign_tools
            # sign_prefix is required for sign_tools
            # global; cannot currently be overridden per-component
            BoolVariable('sign_disable', 'Disable codesign', 0),
            ('sign_keychain', 'Keychain that has signatures'),
            ('sign_id_installer', 'Installer signature name'),
            ('sign_id_app', 'Application/Tool signature name'),
            ('sign_prefix', 'codesign identifier prefix'),
            )
    return True


def exists():
    return 1
