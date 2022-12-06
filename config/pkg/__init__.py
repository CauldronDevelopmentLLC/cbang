'''Builds an OSX single application package'''

import os
import shutil

from SCons.Script import *

deps = ['codesign', 'notarize']


def InstallApps(env, key, target):
    # copy apps, preserving symlinks, no ignores
    for src in env.get(key):
        if isinstance(src, (list, tuple)) and len(src) in (2, 3):
            src_path = src[0]
            dst_path = os.path.join(target, src[1])
        else:
            src_path = src
            dst_path = os.path.join(target, os.path.basename(src))

        shutil.copytree(src_path, dst_path, True)


def build_function(target, source, env):
    # delete old package build dir
    build_dir = 'build/pkg'
    if os.path.exists(build_dir): shutil.rmtree(build_dir)

    # pkg_id
    if env.get('app_id') and not env.get('pkg_id'):
        env.Replace(pkg_id = env.get('app_id') + '.pkg')
    if not env.get('pkg_id'):
        raise Exception('neither pkg_id nor app_id is set')

    # Make root dir
    root_dir = os.path.join(build_dir, 'root')
    os.makedirs(root_dir, 0o755)

    build_dir_packages = os.path.join(build_dir, 'Packages')
    os.makedirs(build_dir_packages, 0o755)

    build_dir_resources = os.path.join(build_dir, 'Resources')
    os.makedirs(build_dir_resources, 0o755)

    # Apps
    if env.get('pkg_apps'):
        d = os.path.join(root_dir, 'Applications')
        os.makedirs(d, 0o755)
        InstallApps(env, 'pkg_apps', d)

    # Other files
    if env.get('pkg_files'):
        env.InstallFiles('pkg_files', root_dir)

    pkg_resources = env.get('pkg_resources')
    if pkg_resources:
        if not isinstance(pkg_resources, (list, tuple)):
            pkg_resources = [[pkg_resources, '.']]
        env.CopyToPackage(pkg_resources, build_dir_resources)

    sign_apps = env.get('sign_apps', [])
    for path in sign_apps:
        if not path.startswith('/'):
            path = os.path.join(root_dir, path)
        env.SignApplication(path)

    sign_tools = env.get('sign_tools', [])
    for path in sign_tools:
        if not path.startswith('/'):
            path = os.path.join(root_dir, path)
        env.SignExecutable(path)

    # build component pkg
    cmd = ['${PKGBUILD}',
           '--root', root_dir,
           '--id', env.get('pkg_id'),
           '--version', env.get('version'),
           '--install-location', env.get('pkg_install_to', '/'),
           ]
    if env.get('pkg_scripts'): cmd += ['--scripts', env.get('pkg_scripts')]
    if env.get('pkg_plist'): cmd += ['--component-plist', env.get('pkg_plist')]
    cmd += [build_dir_packages + '/%s.pkg' % env.get('package_name')]

    env.RunCommandOrRaise(cmd)

    # Filter distribution.xml
    dist = None
    if env.get('pkg_distribution'):
        with open(env.get('pkg_distribution'), 'r') as f: data = f.read()
        dist = build_dir + '/distribution.xml'
        with open(dist, 'w') as f: f.write(data % env)
    # TODO else build distribution.xml similar to flastdistpkg

    # productbuild command
    cmd = ['${PRODUCTBUILD}']
    if dist:
        cmd += ['--distribution', dist]
        cmd += ['--package-path', build_dir_packages]
        cmd += ['--resources', build_dir_resources]
    else:
        print("WARNING: No distribution specified. Attempting to build "
              "using --root. Package will not have Resources and will put "
              "wrong package id in receipts.")
        cmd += ['--root', root_dir, env.get('pkg_install_to', '/'),
                '--id', env.get('pkg_id'),
                '--version', env.get('version')]
        scripts = env.get('pkg_scripts')
        if scripts: cmd += ['--scripts', scripts]

    if env.get('sign_id_installer') and not env.get('sign_disable'):
        cmd += ['--sign', env.get('sign_id_installer')]
        if env.get('sign_keychain'):
            cmd += ['--keychain', env.get('sign_keychain')]

    cmd += [str(target[0])]

    env.RunCommandOrRaise(cmd)

    # write package-description.txt before notarize, which may raise
    desc = env.get('short_description', '').strip()
    if not desc: desc = env.get('description', '').strip()
    if not desc: desc = env.get('summary', '').strip()
    # note: desc may be '', write anyway
    print('writing package-description.txt')
    env.WriteStringToFile('package-description.txt', desc)

    # don't try to notarize if nothing was signed
    if sign_apps or sign_tools:
        env.NotarizeWaitStaple(str(target[0]), timeout = 1200)


def generate(env):
    env.SetDefault(PKGBUILD = 'pkgbuild')
    env.SetDefault(PRODUCTBUILD = 'productbuild')

    bld = Builder(action = build_function,
                  source_factory = SCons.Node.FS.Entry,
                  source_scanner = SCons.Defaults.DirScanner)
    env.Append(BUILDERS = {'Pkg' : bld})

    for tool in deps: env.CBLoadTool(tool)

    return True


def exists():
    return 1
