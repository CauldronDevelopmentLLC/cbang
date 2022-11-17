''' notarize '''

# TODO remove support for altool when it stops working in Fall 2023

import os
import sys
import subprocess
import re
import time
try:
    import json
except ImportError:
    import simplejson as json

from SCons.Script import *

deps = ['codesign'] # uses some sign_ vars


def notarize_get_identifier(env, fpath):
    # construct identifier from sign_prefix and pkg file name
    sign_prefix = env.get('sign_prefix')
    identifier = os.path.basename(fpath)
    # strip .zip
    if identifier.endswith('.zip'):
        identifier = os.path.splitext(identifier)[0]
    # strip .xip
    if identifier.endswith('.xip'):
        identifier = os.path.splitext(identifier)[0]
    # replace .mpkg with .pkg, else strip .app
    if identifier.endswith('.mpkg'):
        identifier = os.path.splitext(identifier)[0] + '.pkg'
    elif identifier.endswith('.app'):
        identifier = os.path.splitext(identifier)[0]

    if sign_prefix:
        if not sign_prefix.endswith('.'):
            sign_prefix += '.'
        identifier = sign_prefix + identifier

    # sanitize identifier
    # strip spaces
    identifier = identifier.replace(' ', '')
    # replace invalid chars with hyphen
    identifier = re.sub(r'[^A-Za-z0-9\-\.]', '-', identifier)

    return identifier


def notarize_sanity_check(env, fpath):
    if env.get('notarize_disable'):
        print('NOTE: notarize_disable is True')
        return
    if env.get('sign_disable'):
        env['notarize_disable'] = True
        print('WARNING: setting notarize_disable because sign_disable is set',
              file=sys.stderr)
        return
    if not os.path.isfile(fpath):
        raise Exception('ERROR: file does not exist: \"%s\"' % fpath)
    # if invaid config disable notarization and print warning
    notarize_profile = env.get('notarize_profile')
    notarize_user = env.get('notarize_user')
    notarize_pass = env.get('notarize_pass')
    if not (notarize_profile or (notarize_user and notarize_pass)):
        env['notarize_disable'] = True
        print('WARNING: setting notarize_disable due to incomplete config',
              file=sys.stderr)
        if not notarize_profile:
            print('NOTE: notarize_profile is not set', file=sys.stderr)
        if not notarize_user:
            print('NOTE: notarize_user is not set', file=sys.stderr)
        if not notarize_pass:
            print('NOTE: notarize_pass is not set', file=sys.stderr)
    if not env.get('sign_prefix'):
        print('WARNING: sign_prefix is not set', file=sys.stderr)


def notarize_get_status(env, requestUUID):
    notarize_user = env.get('notarize_user')
    notarize_pass = env.get('notarize_pass')
    rstatus = None
    cmd = ['/usr/bin/xcrun', 'altool',
           '--notarization-info', requestUUID,
           '--username', notarize_user,
           '--password', notarize_pass,
           '--output-format', 'json']
    # run cmd, catch json output string
    output = subprocess.check_output(cmd)
    # get dict from json string
    d = json.loads(output)
    # extract request status string
    d2 = d.get('notarization-info', {})
    rstatus = d2.get('Status')
    return rstatus


def notarize_print_info(env, requestUUID):
    notarize_user = env.get('notarize_user')
    notarize_pass = env.get('notarize_pass')
    cmd = ['/usr/bin/xcrun', 'altool',
           '--notarization-info', requestUUID,
           '--username', notarize_user,
           '--password', notarize_pass]
    # run cmd, letting output go to stdout/stderr, ignore exit code
    subprocess.call(cmd)


def notarize_request(env, fpath, ident):
    notarize_user = env.get('notarize_user')
    notarize_pass = env.get('notarize_pass')
    notarize_asc = env.get('notarize_asc')
    notarize_team = env.get('notarize_team')
    requestUUID = None
    cmd = ['/usr/bin/xcrun', 'altool',
           '--notarize-app',
           '--primary-bundle-id', ident,
           '--username', notarize_user,
           '--password', notarize_pass,
           '--output-format', 'json']
    if notarize_asc:
        cmd += ['--asc-provider', notarize_asc]
    elif notarize_team:
        cmd += ['--team-id', notarize_team]
    cmd += ['--file', fpath]
    # run cmd, catch json output string
    print('Uploading ' + fpath + ' for notarization')
    print('@', cmd)
    output = subprocess.check_output(cmd)
    # get dict from json string
    d = json.loads(output)
    msg = d.get('success-message')
    if msg:
        print(msg)
    d2 = d.get('notarization-upload', {})
    requestUUID = d2.get('RequestUUID')
    return requestUUID


def notarize_and_wait(env, fpath, ident, timeout, interval):
    # request notarization
    requestUUID = notarize_request(env, fpath, ident)
    print('Notarization RequestUUID: ' + requestUUID)
    if not requestUUID:
        raise Exception('could not upload for notarization')

    # wait for status to not be "in progress"
    status = 'in progress'
    elapsed = 0
    while status == 'in progress':
        print('waiting...', end='')
        sys.stdout.flush()
        time.sleep(interval)
        status = notarize_get_status(env, requestUUID)
        print(status)
        elapsed += interval
        if timeout and (elapsed >= timeout):
            break

    notarize_print_info(env, requestUUID)

    if (timeout > 0) and (status == 'in progress'):
        print('warning: exceeded ' + timeout +
              ' seconds waiting for status to change',
              file=sys.stderr)
        return status
    if status != 'success':
        raise Exception('Could not notarize ' + fpath)

    return status


def notarize_staple_file(fpath):
    cmd = ['/usr/bin/xcrun', 'stapler', 'staple', fpath]
    print('Stapling ' + fpath)
    # Maybe check exit code; loop if fail until timeout
    # Note: failure to staple may not be fatal, assuming
    # the notarization request will be approved sometime later.
    # Stapling just allows offline validation by Gatekeeper.
    # It would be bad for staple to fail when status was "success".
    ret = subprocess.call(cmd)
    if ret != 0:
        print('WARNING: unable to staple ' + fpath, file=sys.stderr)
    return ret


def NewNotarizeWaitStaple(env, fpath, timeout=0):
    notarize_sanity_check(env, fpath)
    if env.get('sign_disable'): return False
    if env.get('notarize_disable'): return False
    if timeout < 0: timeout = 0
    elif timeout > 3600: timeout = 3600
    notarize_profile = env.get('notarize_profile')
    cmd = ['/usr/bin/xcrun', 'notarytool', 'submit', '--wait']
    if timeout: cmd += ['--timeout', str(timeout)]
    cmd += ['-p', notarize_profile, fpath]
    print('@', cmd)
    ret = subprocess.call(cmd)
    if ret != 0:
        raise Exception('ERROR: unable to notarize \"%s\"' % fpath)
    ret = notarize_staple_file(fpath)
    if ret != 0:
        print('ERROR: Stapling failed, though notarize may have succeeded',
              file=sys.stderr)
    print('Done notarizing.')
    return ret == 0


def NotarizeWaitStaple(env, fpath, timeout=0, interval=30):
    if env.get('notarize_profile'):
        return NewNotarizeWaitStaple(env, fpath, timeout)
    notarize_sanity_check(env, fpath)
    if env.get('sign_disable'):
        return False
    if env.get('notarize_disable'):
        return False
    identifier = notarize_get_identifier(env, fpath)
    print('Notarization identifier: \"%s\"' % identifier)
    if timeout < 0:
        timeout = 0
    elif timeout > 3600:
        timeout = 3600
    if interval < 10:
        interval = 10
    elif interval > 60:
        interval = 60
    # will raise or return status "success" or "in process"
    status = notarize_and_wait(env, fpath, identifier, timeout, interval)
    # try once to staple; failure is not fatal; could still be in process
    # note that notarization in process may ultimately fail
    ret = notarize_staple_file(fpath)
    if status == 'success' and ret != 0:
        print('ERROR: Stapling failed, though notarize succeeded',
              file=sys.stderr)
    print('Done notarizing.')
    return ret == 0


def generate(env):
    if env['PLATFORM'] == 'darwin' or int(env.get('cross_osx', 0)):
        env.CBAddVariables(
            # set these vars in scons-options.py
            # sign_prefix is prepended tp pkg filename to make identifier
            # You should at least disable notarization for debug builds
            # Really, only pkgs users can download need be notarized
            BoolVariable('notarize_disable', 'Disable notarization', 0),
            ('notarize_profile', 'The notarytool keychain item profile name'),
            # if using notarize_profile, you do not need user, pass, asc, team
            ('notarize_user', 'The deveoper AppleID email used to notarize.' +
             ' Need not be same as codesign account.'),
            # Example pass: '@keychain:Developer altool: Jane Doe'
            # The actual pass would be in a keychain item with
            # arbitrary label 'Developer altool: Jane Doe'.
            # In Keychain Access, this seems to be "Where", not "Name".
            # The first time notarization is attempted, you may be asked
            # to "Always Allow" altool to access keychain item.
            # See "man altool" for other password options.
            ('notarize_pass', 'The altool app-specific password'),
            # The 10-character asc and team are allegedly optional if
            # there is only one developer in notarize_user account.
            # If both are specified, only asc is used.
            ('notarize_asc', 'The developer asc provider id'),
            ('notarize_team', 'The developer team id'),
            )

        env.AddMethod(NotarizeWaitStaple)

    for tool in deps:
        env.CBLoadTool(tool)

    return True


def exists():
    return 1
