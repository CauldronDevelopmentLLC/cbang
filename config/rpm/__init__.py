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


def install_files(f, env, key, build_dir, path, prefix = None, perms = None,
                  dperms = 0o755):
    if perms is None: perms = 0o644

    if key in env:
        target = build_dir + path

        # Copy
        env.CopyToPackage(env.get(key), target, perms, dperms)

        # Write files list
        for src, dst, mode in env.ResolvePackageFileMap(env.get(key), target):
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

    # Create directories needed by rpmbuild
    for dir in ['BUILD', 'BUILDROOT', 'RPMS', 'SOURCES', 'SPECS', 'SRPMS']:
        dir = 'build/' + dir
        if os.path.exists(dir): shutil.rmtree(dir)
        os.makedirs(dir)

    # SPEC's %install section is responsible for populating BUILDROOT
    # (aka %{buildroot} or $RPM_BUILD_ROOT)
    build_dir = 'build/BUILD'

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
        write_var(env, f, 'Vendor', 'vendor')
        write_var(env, f, 'Packager', 'maintainer')
        write_var(env, f, 'Icon', 'icon')
        write_var(env, f, 'Prefix', 'prefix')
        #write_var(env, f, 'BuildArch', 'package_arch', env.GetPackageArch())
        write_var(env, f, 'Provides', 'rpm_provides', multi = True)
        write_var(env, f, 'Conflicts', 'rpm_conflicts', multi = True)
        write_var(env, f, 'Obsoletes', 'rpm_obsoletes', multi = True)
        write_var(env, f, 'BuildRequires', 'rpm_build_requires', multi = True)
        write_var(env, f, 'Requires', 'rpm_requires', multi = True)
        write_var(env, f, 'Requires(pre)', 'rpm_pre_requires', multi = True)
        write_var(env, f, 'Requires(preun)', 'rpm_preun_requires', multi = True)
        write_var(env, f, 'Requires(post)', 'rpm_post_requires', multi = True)
        write_var(env, f, 'Requires(postun)', 'rpm_postun_requires',
                  multi = True)

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

        for files in [
            ['documents', '/usr/share/doc/' + name, '%doc', None],
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

    # rpmbuild strips debug information from binaries by default if %build and
    # %install sections are present (may be empty)
    if int(env.get('debug')):
        cmddebug = ' --define "__strip /usr/bin/true"'
    else: cmddebug = ''

    # Build the package
    target = str(target[0])
    cmd = 'rpmbuild -bb --define "_topdir %s/build" --define "_rpmfilename %s"' \
          '%s --target %s %s' % (
          os.getcwd(), target, cmddebug, env.GetPackageArch(), spec_file)
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
