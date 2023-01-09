'''  flatdistpkg '''

import os
import shutil
import glob
import re
import config
import zipfile
import mimetypes
mimetypes.init()

from SCons.Script import *

import glob
try:
    import json
except ImportError:
    import simplejson as json

deps = ['codesign', 'notarize', 'compiler']

filename_package_desc_txt = 'package-description.txt'
filename_package_info_json = 'package-parameters.json'
filename_distribution_xml = 'distribution.xml'

build_dir = 'build/flatdistpkg'
build_dir_tmp = build_dir + '/tmp'
build_dir_resources = build_dir + '/Resources'
build_dir_packages = build_dir + '/Packages'
build_dir_stage = build_dir + '/stage' # appended with /package_name

build_dirs = [
    build_dir_tmp, build_dir_resources,
    build_dir_packages, build_dir_stage,
    ]

# probably don't need these; info should be passed via FlatDistPackager()
default_src_resources = 'Resources'
# going to generate this from scratch, so won't need template
default_src_distribution_xml = 'osx/distribution.xml'

build_dir_distribution_xml = \
    os.path.join(build_dir_tmp, filename_distribution_xml)


def clean_old_build(env):
    # rm intermediate build stuff
    # do not rm old build products, descriptions
    if os.path.exists(build_dir):
        shutil.rmtree(build_dir)


def setup_dirs(env):
    return
    # FUTURE USE
    # build/distpkg/package_name/{tmp,Resources,Packages}
    # build/flatpkg/package_name/{root,Resources,Scripts}
    # useful if a project creates multiple distpkg
    # clean_old_build would need to only rm build/distpkg/thisname
    # would also want to separate flatpkg building
    name = env['package_name']
    env['build_dir'] = 'build'
    env['build_dir_tmp'] = tmp = \
        build_dir + '/distpkg/' + name + '/tmp'
    env['build_dir_resources'] = res = \
        build_dir + '/distpkg/' + name + '/Resources'
    env['build_dir_packages'] = packs = \
        build_dir + '/distpkg/' + name + '/Packages'
    env['build_dir_stage'] = stage = build_dir + '/flatpkg'
    env['build_dirs'] = [tmp,res,packs,stage]
    env['build_dir_distribution_xml'] = \
        os.path.join(tmp, filename_distribution_xml)


def create_dirs(env):
    #setup_dirs(env)
    dirs = build_dirs #+ env.get('build_dirs',[])
    for d in dirs:
        if not os.path.isdir(d):
            os.makedirs(d, 0o755)
    #env.Dir(build_dir)
    # above fails with
    # scons: *** [fah-installer_7.2.12_intel.pkg.zip] TypeError :
    #   Tried to lookup File 'build' as a Dir.


def rename_prepostflight_scripts(scripts_dir):
    # unless PackageInfo says otherwise,
    # flat pkgs only use pre/postinstall
    pre1 = os.path.join(scripts_dir, 'preflight')
    pre2 = os.path.join(scripts_dir, 'preinstall')
    post1 = os.path.join(scripts_dir, 'postflight')
    post2 = os.path.join(scripts_dir, 'postinstall')
    if os.path.isfile(pre1) and not os.path.exists(pre2):
        print('renaming %s to %s' % (pre1, pre2))
        os.rename(pre1, pre2)
    if os.path.isfile(post1) and not os.path.exists(post2):
        print('renaming %s to %s' % (post1, post2))
        os.rename(post1, post2)


def build_component_pkg(info, env):
    # FIXME -- possibly incomplete and makes assumptions
    # build component from info using pkgbuild
    # uses globals build_dir_stage, build_dir_packages
    name = info.get('name')
    home = info.get('home')
    pkg_id = info.get('pkg_id')
    # if no component version, use package version
    version = info.get('version', env.get('version'))
    root = info.get('root')
    resources = info.get('resources')
    pkg_resources = info.get('pkg_resources')
    install_to = info.get('install_to', env.get('pkg_install_to', '/'))
    pkg_nopayload = info.get('pkg_nopayload', False)

    if not name:
        raise Exception('component has no name')
    if not home:
        raise Exception('component %s has no home' % name)
    if not pkg_id:
        raise Exception('component %s has no pkg_id' % name)
    if not version:
        raise Exception('no version for component %s' % name)

    stage = os.path.join(build_dir_stage, name)
    stage_resources = os.path.join(stage, 'Resources')

    target = os.path.join(build_dir_packages, name + '.pkg')

    # FIXME uses knowledge of how pkg.py works
    if not root:
        root = home + '/build/pkg/root'
    if not resources:
        resources = home + '/build/pkg/Resources'

    # FIXME uses knowledge of layout of most projects
    # not very important, as component pkgs have no resources
    if not pkg_resources:
        d = os.path.join(home,'osx/Resources')
        if d and os.path.isdir(d):
            pkg_resources = [[d, '.']]

    # FIXME uses knowledge of how pkg_scripts has been used
    # doesn't handle pkg_scripts as tuple list, like pkg_resources always is
    scripts = os.path.join(home, info.get('pkg_scripts', 'osx/scripts'))
    stage_scripts = None
    if scripts:
        scripts_dir_name = os.path.basename(scripts)
        stage_scripts = os.path.join(stage, scripts_dir_name)

    # make needed dirs
    if not os.path.isdir(stage): os.makedirs(stage)

    # if pkg_files exists, always copy to root, same as pkg module does
    # Note that this creates root if it doesn't yet exist
    pkg_files = info.get('pkg_files')
    if pkg_files and not pkg_nopayload:
        env.CopyToPackage(pkg_files, root)

    if not os.path.isdir(root) and not pkg_nopayload:
        raise Exception('%s component root does not exist! %s' % (name, root))

    # try to copy scripts to our stage and use that to avoid most cruft
    # also allows renaming scripts before pkgbuild
    if scripts and os.path.isdir(scripts):
        env.CopyToPackage(scripts, stage, perms = 0o755)

    # copy resources to our stage for future distpkg use
    if resources and os.path.isdir(resources):
        env.CopyToPackage(resources, stage)
    elif pkg_resources:
        env.CopyToPackage(pkg_resources, stage_resources)

    # pkg_plist might specify non-default script names
    if not env.get('pkg_plist') and not info.get('pkg_plist'):
        rename_prepostflight_scripts(stage_scripts)

    # if any apps/tools should be codesign'd do that now
    # assumes project didn't do it when creating its distroot
    # TODO clone env and add info so all sign_ vars can be overridden
    paths = []
    for path in info.get('sign_apps', []):
      if '*' in path: paths += glob.glob(path, root_dir = root)
      else: paths.append(path)
    for path in paths:
        if not path.startswith('/'): path = os.path.join(root, path)
        env.SignApplication(path)

    paths = []
    for path in info.get('sign_tools', []):
      if '*' in path: paths += glob.glob(path, root_dir = root)
      else: paths.append(path)
    for path in paths:
        if not path.startswith('/'): path = os.path.join(root, path)
        env.SignExecutable(path)

    cmd = ['pkgbuild',
        '--install-location', install_to,
        '--version', version,
        '--identifier', pkg_id,
        ]

    if pkg_nopayload:
        cmd += ['--nopayload']
    else:
        cmd += ['--root', root]

    if stage_scripts and os.path.isdir(stage_scripts):
        cmd += ['--scripts', stage_scripts]
    elif scripts and os.path.isdir(scripts):
        cmd += ['--scripts', scripts]
    cmd += [target]
    env.RunCommandOrRaise(cmd)


def build_component_pkgs(env):
    components = env.get('distpkg_components', None)

    if not components:
        # TODO: instead create components for single-pkg dist
        # and set any needed distpkg_/pkg_ vars
        raise Exception('no distpkg_components specified')

    # validate and fill-in any missing info
    for info in components:
        # home is required
        home = info.get('home')
        if not home:
            name = info.get('name') # might be None at this point
            raise Exception('home not provided for component ' + name)
        # name and pkg_id are also required, but may be in json

        # try to load pkg info json, if exists
        # merge with components info
        # FIXME UNTESTED AND MAYBE INCOMPLETE
        if not info.get('params_json_loaded'):
            jsonfile = os.path.join(home, filename_package_info_json)
            if os.path.isfile(jsonfile):
                print('loading info from %s' % jsonfile)
                info2 = json.load(jsonfile)
                # merge
                # trust what we were passed over what json says
                # this allows distpkg to override component pkg info
                info2.update(info)
                info = info2
                info['params_json_loaded'] = True

        name = info.get('name')
        if not info.get('package_name'):
            info['package_name'] = name
        if not info.get('package_name_lower'):
            info['package_name_lower'] = name.lower()

        # try to get description, if still none
        desc = info.get('short_description', '').strip()
        if not desc:
            desc = info.get('description', '').strip()
        if not desc:
            # try contents of <home>/package-description.txt
            fname = os.path.join(home, filename_package_desc_txt)
            if os.path.isfile(fname):
                with open(fname, 'r') as f: desc = f.read().strip()
        if not desc:
            desc = info.get('summary', '').strip()
        info['short_description'] = desc

    for info in components:
        try:
            build_component_pkg(info, env)
        except Exception as e:
            print('unable to build component ' + info.get('name'))
            raise e

    # replace distpkg_components with modified/created info list
    env['distpkg_components'] = components


def build_product_pkg(target, source, env):
    version = env.get('version')
    if not version: raise Exception('version is not set')
    pkg_id = env.get('distpkg_id')
    cmd = ['productbuild',
        '--distribution', build_dir_distribution_xml,
        '--package-path', build_dir_packages,
        '--resources', build_dir_resources,
        '--version', version,
        ]
    if pkg_id: cmd += ['--identifier', pkg_id]
    cmd += [target]
    env.RunCommandOrRaise(cmd)


def expand_flat_pkg(target, source, env):
    cmd = ['pkgutil', '--expand', source, target]
    env.RunCommandOrRaise(cmd)


def flatten_to_pkg(target, source, env):
    cmd = ['pkgutil', '--flatten', source, target]
    env.RunCommandOrRaise(cmd)


def build_or_copy_distribution_template(env):
    # TODO: use env pkg_distribution if exists; escape values to insert into it
    target = build_dir_distribution_xml
    source = default_src_distribution_xml

    build_distribution_template(env, target)

    if os.path.exists(target): return # assume build succeeded

    print('WARNING: did not generate distribution.xml; '
          'using pre-built template %s' % source)
    if not os.path.isfile(source):
        raise Exception('pre-built template does not exist %s' % source)
    #Execute(Copy(target, source))
    # sometimes after a 'scons --clean', above fails with
    # scons: *** [fah-installer_7.2.12_intel.pkg.zip] TypeError :
    #   Tried to lookup File 'build' as a Dir.
    # TODO: filter it, don't just copy
    shutil.copy2(source, target)


def build_distribution_template(env, target=None):
    if not target:
        target = build_dir_distribution_xml

    print('generating ' + target)

    distpkg_target = env.get('distpkg_target', '10.5')
    ver = tuple([int(x) for x in distpkg_target.split('.')])
    distpkg_arch = env.get('distpkg_arch')
    if not distpkg_arch:
        distpkg_arch = env.get('osx_archs')

    # handle some aberrant values
    if distpkg_arch in (None, '', 'intel'):
        print('using distpkg_arch i386 instead of "%s"' % distpkg_arch)
        distpkg_arch = 'i386'
    # some projects might be passing package_arch
    if distpkg_arch == 'universal':
        distpkg_arch = 'x86_64 arm64'

    hostArchitectures = ','.join(distpkg_arch.split())

    # generate new tree
    import xml.etree.ElementTree as etree
    root = etree.Element('installer-gui-script', {'minSpecVersion':'2'})
    tree = etree.ElementTree(root)

    title = env.get('summary', env.get('package_name'))
    title = title.replace('\\n','\n').replace('\\t','\t').replace('\\"','"')
    etree.SubElement(root, 'title').text = title

    # non-root pkg installs are very buggy, so this should stay True
    distpkg_root_volume_only = env.get('distpkg_root_volume_only', True)
    allow_external = env.get('distpkg_allow_external_scripts', False)
    if allow_external: allow_external = 'yes'
    else: allow_external = 'no'
    allow_ppc = 'ppc' in distpkg_arch.lower() and ver < (10,6)
    if distpkg_root_volume_only: rootVolumeOnly = 'true'
    else: rootVolumeOnly = 'false'
    opts = {
        'allow-external-scripts': allow_external,
        'customize': env.get('distpkg_customize', 'allow'),
        # i386 covers both 32 and 64 bit kernel, NOT cpu
        # cpu 64 bit check must be done in script
        'hostArchitectures': hostArchitectures,
        'rootVolumeOnly': rootVolumeOnly,
        }
    etree.SubElement(root, 'options', opts)

    # WARNING domains element can be buggy for anything other than root-only.
    if (10,13) <= ver and distpkg_root_volume_only:
        etree.SubElement(root, 'domains', {
            'enable_anywhere': 'false',
            'enable_currentUserHome': 'false',
            'enable_localSystem': 'true',
            })

    for key in 'welcome license readme conclusion'.split():
        if 'distpkg_' + key in env:
            etree.SubElement(root, key, {'file': env.get('distpkg_' + key)})

    background = env.get('distpkg_background', None)
    if background:
        mtype = mimetypes.guess_type(background)[0]
        etree.SubElement(root, 'background', {
            'alignment': 'bottomleft',
            'file': background,
            'mime-type': mtype,
            'scaling': 'none',
            })
    # dark mode background; defaults to same image as light
    background = env.get('distpkg_background_dark', background)
    if background:
        mtype = mimetypes.guess_type(background)[0]
        etree.SubElement(root, 'background-darkAqua', {
            'alignment': 'bottomleft',
            'file': background,
            'mime-type': mtype,
            'scaling': 'none',
            })

    ic = etree.SubElement(root, 'installation-check', {
        'script': 'install_check();'})
    vc = etree.SubElement(root, 'volume-check', {
        'script': 'volume_check();'})

    if distpkg_target:
        e = etree.Element('allowed-os-versions')
        etree.SubElement(e, 'os-version', {'min':distpkg_target})
        vc.append(e)

    # options hostArchitectures is sufficient on 10.5+, but no clear
    # message is given by Installer.app, so we still want potential
    # hw.cputype check in install_check
    # allowed-os-versions req macOS 10.6.6+, so volume_check is still needed
    script_text = """
function is_min_version(ver) {
  if (my.target && my.target.systemVersion &&
    system.compareVersions(my.target.systemVersion.ProductVersion, ver) >= 0)
    return true;
  return false;
}
function have_64_bit_cpu() {
  if (system.sysctl('hw.optional.x86_64'))
    return true;
  if (system.sysctl('hw.cpu64bit_capable'))
    return true;
  if (system.sysctl('hw.optional.64bitops'))
    return true;
  return false;
}
function volume_check() {
  if (!is_min_version('""" + distpkg_target + """')) {
    my.result.title = 'Unable to Install';
    my.result.message = 'macOS """ + distpkg_target + \
    """ or later is required.';
    my.result.type = 'Fatal';
    return false;
  }
  return true;
}
function install_check() {"""

    if not allow_ppc and ver < (10,6,6):
        script_text += """
  if (system.sysctl('hw.cputype') == '18') {
    my.result.title = 'Unable to Install';
    my.result.message = 'PowerPC Mac is not supported.';
    my.result.type = 'Fatal';
    return false;
  }"""

    if distpkg_arch == 'x86_64':
        script_text += """
  if (have_64_bit_cpu() == false) {
    my.result.title = 'Unable to Install';
    my.result.message ='A 64-bit processor is required (Core 2 Duo or better).';
    my.result.type = 'Fatal';
    return false;
  }"""

    script_text += """
  return true;
}
""" # end install_check

    outline = etree.SubElement(root, 'choices-outline')

    components = env.get('distpkg_components')
    pkgrefs = []
    for info in components:
        pkg_id = info.get('pkg_id')
        name_lower = info.get('package_name_lower')
        pkg_target = info.get('pkg_target', distpkg_target)
        pkg_ver = tuple([int(x) for x in pkg_target.split('.')])
        choice_id = name_lower
        # remove any dots or spaces in choice_id for 10.5 compatibility
        choice_id = choice_id.replace('.','').replace(' ','')
        title = info.get('package_name', info.get('summary'))
        title = title.replace('\\n','\n').replace('\\t','\t').replace('\\"','"')
        desc = info.get('short_description', info.get('description', ''))
        desc = desc.replace('\\n','\n').replace('\\t','\t').replace('\\"','"')
        desc = desc.replace('\r\n', '\n').replace('\n', '__CR__')
        etree.SubElement(outline, 'line', {'choice':choice_id})
        choice = etree.SubElement(root, 'choice', {
                'id': choice_id,
                'title': title,
                'description': desc,
                })
        etree.SubElement(choice, 'pkg-ref', {'id': pkg_id})
        pkg_path = info.get('package_name') + '.pkg'
        pkg_ref_info =  {
            'id': pkg_id,
            # version and installKBytes will be added by productbuild
            }
        if distpkg_root_volume_only or info.get('pkg_root_volume_only'):
            pkg_ref_info['auth'] = 'Root'
        ref = etree.Element('pkg-ref', pkg_ref_info)
        ref.text = pkg_path
        pkgrefs.append(ref)
        must_close_apps = info.get('must_close_apps', [])
        if must_close_apps:
            r = etree.Element('pkg-ref', {'id':pkg_id})
            must_close = etree.SubElement(r, 'must-close')
            for app_id in must_close_apps:
                etree.SubElement(must_close, 'app', {'id':app_id})
            pkgrefs.append(r)
        # if appropriate, add {enabled,selected} attrs and script
        if pkg_ver > ver:
            # TODO add arch checks if component has diff reqs than distpkg
            munged_name = 'is_' + re.sub(r'[^A-Za-z0-9]+', '_', name_lower)
            choice.set('enabled', munged_name + '_enabled();')
            choice.set('selected', munged_name + '_enabled();')
            script_text += """
function """ + munged_name + """_enabled() {
  return is_min_version('""" + pkg_target + """');
}
"""
    for ref in pkgrefs: root.append(ref)

    etree.SubElement(root, 'script').text = '__SCRIPT_TEXT__'
    script_text = '<![CDATA[\n' + script_text + '\n]]>'

    # save tree to target
    try: etree.indent(tree)
    except: pass
    # do NOT use 'utf-8' for ElementTree.write()
    with open(target, 'w') as f: tree.write(f, encoding = 'unicode')
    with open(target, 'r') as f: contents = f.read()
    contents = contents.replace('__SCRIPT_TEXT__', script_text)
    with open(target, 'w') as f: f.write(contents)


def patch_expanded_pkg_distribution(target, source, env):
    # fixup whatever needs changing for < 10.6.6 compatibility
    distpkg_target = env.get('distpkg_target', '10.5')
    ver = tuple([int(x) for x in distpkg_target.split('.')])
    if ver >= (10,6,6):
        return

    fpath = os.path.join(target, 'Distribution')
    # load xml
    tree = None
    if os.path.isfile(fpath):
        import xml.etree.ElementTree as etree
        tree = etree.parse(fpath)
    # make changes
    if tree:
        print('patching ' + fpath)
        root = tree.getroot()
        if root.tag != 'installer-script':
            print('changing root tag from %s to installer-script' % root.tag)
            root.tag = 'installer-script'
        minSpecVersion = root.get('minSpecVersion')
        if minSpecVersion != '1':
            print('changing minSpecVersion from %s to 1' % minSpecVersion)
            root.set('minSpecVersion', '1')
        # overwrite back to fpath
        with open(fpath, 'w') as f: tree.write(f, encoding = 'unicode')
        # FIXME detect any failures somehow
        return
    # failed to load xml
    print('WARNING: unable to load and patch %s' % fpath)
    print('WARNING: pkg may require OSX 10.6.6 or later')


def patch_expanded_pkg_distribution_cr(target, env):
    # replace __CR__ with &#13; in Distribution
    fpath = os.path.join(target, 'Distribution')
    if not os.path.isfile(fpath):
        raise Exception('Distribution xml file does not exist:', fpath)
    with open(fpath, 'r') as f: contents = f.read()
    contents = str(contents).replace('__CR__', '&#13;')
    with open(fpath, 'w') as f: f.write(contents)


def flat_dist_pkg_build(target, source, env):
    # expect target one of
    #  package_name_version_arch.pkg
    #  package_name_version_arch.mpkg.zip
    #  package_name_version_arch.pkg.zip
    target_pkg = str(target[0])
    target_zip = None
    if target_pkg.endswith('.zip'):
      target_zip = target_pkg
      target_pkg = os.path.splitext(target_pkg)[0]
    # validate target, source, env
    # tolerate .mpkg if also .zip
    target_base, ext = os.path.splitext(target_pkg)
    if not ext in ('.pkg', '.mpkg'):
        raise Exception('Expected .pkg or .mpkg in package name')
    if ext == '.mpkg' and not target_zip:
        raise Exception('Cannot create .mpkg without .zip')

    # .mpkg causes errors in log, and does not work on 10.5,
    # so use .pkg for flat package, even if final target is .mpkg.zip
    if ext == '.mpkg': target_pkg = target_base + '.pkg'

    # flat packages require OS X 10.5+
    distpkg_target = env.get('distpkg_target', '10.5')
    ver = tuple([int(x) for x in distpkg_target.split('.')])
    if ver < (10,5):
        raise Exception(
            'incompatible configuration: flat package and osx pre-10.5 (%s)'
            % distpkg_target)

    clean_old_build(env)
    create_dirs(env)

    target_pkg_tmp = os.path.join(build_dir_tmp, target_base + '.tmp.pkg')
    target_pkg_expanded = os.path.join(build_dir_tmp, target_base+'.expanded')
    target_unsigned = os.path.join(build_dir_tmp, target_pkg)

    # TODO: more validation here ...

    name = env.get('package_name')
    print('Building "%s", target %s' % (name, distpkg_target))

    # copy our own dist Resources
    if env.get('distpkg_resources'):
        env.InstallFiles('distpkg_resources', build_dir_resources)
    elif env.get('pkg_resources'):
        env.InstallFiles('pkg_resources', build_dir_resources)

    env.UnlockKeychain()

    build_component_pkgs(env)

    # probably not needed
    # I think build_component_pkgs always raises on any failure
    if not env.get('distpkg_components'):
        raise Exception('No distpkg_components. Cannot continue.')

    build_or_copy_distribution_template(env)

    build_product_pkg(target_pkg_tmp, [], env)

    expand_flat_pkg(target_pkg_expanded, target_pkg_tmp, env)

    patch_expanded_pkg_distribution(target_pkg_expanded, [], env)
    patch_expanded_pkg_distribution_cr(target_pkg_expanded, env)

    # if built any components using home/osx/scripts, we still need to rename
    for d in glob.glob(os.path.join(target_pkg_expanded, '*.pkg/Scripts')):
        rename_prepostflight_scripts(d)

    flatten_to_pkg(target_unsigned, target_pkg_expanded, env)

    env.SignOrCopyPackage(target_pkg, target_unsigned)

    env.NotarizeWaitStaple(target_pkg, timeout = 1200)

    if target_zip:
        env.ZipDir(target_zip, target_pkg)

    # write package-description.txt
    # currently also needlessly generated by
    # {client,viewer}/SConstruct and control/setup.py
    # it is written by both pkg and flatdistpkg modules
    desc = env.get('short_description', '').strip()
    if not desc:
        desc = env.get('description', '').strip()
    if not desc:
        desc = env.get('summary', '').strip()
    # note: desc may be '', write anyway
    print('writing ' + filename_package_desc_txt)
    env.WriteStringToFile(filename_package_desc_txt, desc)


def generate(env):
    bld = Builder(action = flat_dist_pkg_build,
                  source_factory = SCons.Node.FS.Entry,
                  source_scanner = SCons.Defaults.DirScanner)
    env.Append(BUILDERS = {'FlatDistPkg' : bld})
    for tool in deps:
        env.CBLoadTool(tool)
    return True


def exists():
  return 1
