################################################################################
#                                                                              #
#         This file is part of the C! library.  A.K.A the cbang library.       #
#                                                                              #
#               Copyright (c) 2021-2025, Cauldron Development  Oy              #
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

import os
import shutil

from SCons.Script import *
from SCons.Action import CommandAction


def replace_dash(s):
    return s.replace('-', '_')


def write_spec_text_section(f, env, name, var):
    if var in env:
        f.write('%%%s\n%s\n\n' % (name, env.get(var).strip()))


def write_spec_script(f, env, name, var):
    if var in env:
        script = env.get(var)

        with open(script, 'r') as input: contents = input.read().strip()

        f.write('%%%s\n%s\n\n' % (name, contents))

    elif name == 'install':
        f.write('%install\ncp -aT %{cbang_build} %{buildroot}\n\n')


def install_files(f, env, key, build_dir, path, prefix = None, perms = None,
                  dperms = 0o755):
    if perms is None: perms = 0o644

    if key in env:
        target = build_dir + path
        value = env.get(key)

        # Copy
        env.CopyToPackage(value, target, perms, dperms)

        # Write files list
        if key != 'documents':
            for src, dst, mode in env.ResolvePackageFileMap(value, target):
                if prefix is not None: f.write(prefix + ' ')
                f.write(dst[len(build_dir):] + '\n')


def install_dirs(f, env, key, build_dir, prefix = None, dperms = 0o755):
    # Create dirs and write dirs list
    for dir in env.get(key):
        target = build_dir + '/' + dir.lstrip('/')
        print("creating directory %s" % target)
        os.makedirs(target, dperms)
        if prefix: f.write(prefix + ' ')
        else: f.write('%dir ')
        f.write(target[len(build_dir):] + '\n')


def build_function(target, source, env):
    name = env.get('package_name_lower')

    # Create package build dir
    # SPEC's %install section is responsible for populating BUILDROOT (aka
    # %{buildroot} or $RPM_BUILD_ROOT) using our custom %{cbang_build} macro as
    # source
    build_dir = 'build/%s-BUILD' % name
    if os.path.exists(build_dir): shutil.rmtree(build_dir)
    os.makedirs(build_dir)

    # Create directories needed by rpmbuild
    for dir in ['BUILD', 'BUILDROOT', 'RPMS', 'SOURCES', 'SPECS', 'SRPMS']:
        dir = 'build/' + dir
        if os.path.exists(dir): shutil.rmtree(dir)
        os.makedirs(dir)

    # Create the SPEC file
    spec_file = 'build/SPECS/%s.spec' % name
    with open(spec_file, 'w') as f:
        # Create the preamble
        write_var = env.WriteVariable
        write_var(env, f, 'Summary', 'summary')
        write_var(env, f, 'Name', 'package_name_lower')
        write_var(env, f, 'Version', 'version', None, replace_dash)
        write_var(env, f, 'Release', 'package_build', '1', replace_dash)
        write_var(env, f, 'License', 'rpm_license')
        write_var(env, f, 'Group', 'rpm_group')
        write_var(env, f, 'URL', 'url')
        write_var(env, f, 'BugURL', 'bug_url')
        write_var(env, f, 'Vendor', 'vendor')
        write_var(env, f, 'Packager', 'maintainer')
        #write_var(env, f, 'BuildArch', 'package_arch', env.GetPackageArch())
        write_var(env, f, 'Provides', 'rpm_provides', multi = True)
        write_var(env, f, 'Conflicts', 'rpm_conflicts', multi = True)
        write_var(env, f, 'Obsoletes', 'rpm_obsoletes', multi = True)
        write_var(env, f, 'Recommends', 'rpm_recommends', multi = True)
        write_var(env, f, 'Suggests', 'rpm_suggests', multi = True)
        write_var(env, f, 'BuildRequires', 'rpm_build_requires', multi = True)
        write_var(env, f, 'Requires', 'rpm_requires', multi = True)
        write_var(env, f, 'Requires(pre)', 'rpm_pre_requires', multi = True)
        write_var(env, f, 'Requires(preun)', 'rpm_preun_requires', multi = True)
        write_var(env, f, 'Requires(post)', 'rpm_post_requires', multi = True)
        write_var(env, f, 'Requires(postun)', 'rpm_postun_requires',
                  multi = True)

        f.write('\n')

        # Description
        write_spec_text_section(f, env, 'description', 'description')

        # Scripts
        for script in ['prep', 'build', 'install', 'clean', 'pre', 'post',
                       'preun', 'postun', 'verifyscript']:
            write_spec_script(f, env, script, 'rpm_' + script)

        # Files
        if 'rpm_filelist' in env:
            f.write('%%files -f %s\n' % env.get('rpm_filelist'))
        else: f.write('%files\n')

        # See https://bugzilla.redhat.com/show_bug.cgi?id=1113233#c3
        if 'documents' in env:
            f.write('/usr/share/doc/' + name + '\n')

        for files in [
            ['documents', '/usr/share/doc/' + name, None, None],
            ['programs', '/usr/bin', None, 0o755],
            ['scripts', '/usr/bin', None, 0o755],
            ['desktop_menu', '/usr/share/applications', None, None],
            ['init_d', '/etc/init.d', '%config', 0o755],
            ['config', '/etc/' + name, '%config', None],
            ['icons', '/usr/share/pixmaps', None, None],
            ['mime', '/usr/share/mime/packages', None, None],
            ['platform_independent', '/usr/share/' + name, None, None],
            ['misc', '/', None, None],
            ['systemd', '/usr/lib/systemd/system', None, None],
            ]:
            install_files(f, env, files[0], build_dir, files[1], files[2],
                          files[3])

        install_dirs(f, env, 'rpm_client_dirs', build_dir,
                     '%attr(-,' + name + ',' + name + ') %dir')

        # ChangeLog
        write_spec_text_section(f, env, 'changelog', 'rpm_changelog')

    # rpmbuild strips debug information from binaries by default if %install
    # section is present (may be empty)
    if int(env.get('debug')):
        cmddebug = ' --define "__strip /usr/bin/true"'
    else: cmddebug = ''

    # Build the package
    target = str(target[0])
    cmd = 'rpmbuild -bb --define "_topdir %s/build" --define ' \
        '"_rpmfilename %s" --define "cbang_build %s"' '%s --target %s %s' % (
            os.getcwd(), target, os.path.realpath(build_dir), cmddebug,
            env.GetPackageArch(), spec_file)
    CommandAction(cmd).execute(target, [], env)

    # Move the package
    path = 'build/RPMS/' + target
    shutil.move(path, target)


def generate(env):
    bld = Builder(action = build_function,
                  source_factory = SCons.Node.FS.Entry,
                  source_scanner = SCons.Defaults.DirScanner)
    env.Append(BUILDERS = {'RPM' : bld})

    return True


def exists():
    return 1
