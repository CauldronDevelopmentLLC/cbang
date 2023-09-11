import textwrap
import os
import re
import sys
from platform import release
import subprocess
from SCons.Script import *
from collections import OrderedDict


def escstr(s): return s.replace('\\', '\\\\').replace('"', '\\"')


def which(program):
  def is_exe(fpath): return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

  fpath, fname = os.path.split(program)
  if fpath:
    if is_exe(program): return program

  else:
    for path in os.environ["PATH"].split(os.pathsep):
      path = path.strip('"')
      exe_file = os.path.join(path, program)
      if is_exe(exe_file):
        return exe_file

  return None


def svn_get_info():
  svn = which('svn')
  if not svn: return None, None

  revision, branch = None, None

  try:
    p = subprocess.Popen(['svn', 'info'], stderr = subprocess.PIPE,
                         stdout = subprocess.PIPE)
    out, err = p.communicate()
    if isinstance(out, bytes): out = out.decode()

    for line in out.splitlines():
      if line.startswith('Revision: '): revision = line[10:].strip()
      elif line.startswith('URL: '):
        branch = line[5:].strip()
        branch = re.sub(r'https?://[\w\.]+/(svn/)?', '', branch)

  except Exception as e: print(e)

  return revision, branch


def git_get_info():
  if not which('git'): return None, None

  revision, branch = None, None

  try:
    p = subprocess.Popen(['git', 'rev-parse', 'HEAD'],
                         stderr = subprocess.PIPE, stdout = subprocess.PIPE)
    out, err = p.communicate()
    if isinstance(out, bytes): out = out.decode()
    revision = out.strip()

  except Exception as e: print(e)

  try:
    p = subprocess.Popen(['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                         stderr = subprocess.PIPE, stdout = subprocess.PIPE)
    out, err = p.communicate()
    if isinstance(out, bytes): out = out.decode()
    branch = out.strip()

  except Exception as e: print(e)

  return revision, branch


def build_function(target, source, env):
  items = []

  for name in env['BUILD_INFO_PACKAGE_VARS']:
    var = 'PACKAGE_' + name.upper()
    if env.get(var): items.append((name, '"%s"' % env[var]))

  items.append(('Date', '__DATE__'))
  items.append(('Time', '__TIME__'))

  revision, branch = svn_get_info()
  if revision is None: revision, branch = git_get_info()

  if revision is not None:
    items.append(('Revision', '"%s"' % revision))
    items.append(('Branch',   '"%s"' % branch))

  mode     = 'Debug' if env.get('debug', False) else 'Release'
  cflags   = escstr(' '.join(env['CXXFLAGS'] + env['CCFLAGS']))
  platform = sys.platform.lower() + ' ' + release()

  items.append(('Compiler', 'COMPILER'))
  items.append(('Options',  '"%s"' % cflags))
  items.append(('Platform', '"%s"' % platform))
  items.append(('Bits',     'String(COMPILER_BITS)'))
  items.append(('Mode',     '"%s"' % mode))

  target = str(target[0])
  with open(target, 'w') as f:
    note = ('WARNING: This file was auto generated.  Please do NOT '
            'edit directly or check in to source control.')

    f.write(
      '/' + ('*' * 75) + '\\\n   ' +
      '\n   '.join(textwrap.wrap(note)) + '\n' +
      '\\' + ('*' * 75) + '/\n'
      '\n'
      '#include <cbang/Info.h>\n'
      '#include <cbang/String.h>\n'
      '#include <cbang/util/CompilerInfo.h>\n\n'
      'using namespace cb;\n\n'
      )

    ns = env.subst('$BUILD_INFO_NS')
    if ns:
      for namespace in ns.split('::'):
        f.write(env.subst('namespace %s {\n' % namespace))

    f.write(
      '  void addBuildInfo(const char *category) {\n'
      '    Info &info = Info::instance();\n'
      '    info.add(category, true);\n'
    )

    for name, value in items:
      f.write('    info.add(category, "%s", %s);\n' % (name, value))

    f.write('  }\n')

    if ns:
      parts = ns.split('::')
      parts.reverse()
      for namespace in parts:
        f.write('} // namespace %s\n' % namespace)

  return None


def generate(env):
  env.SetDefault(BUILD_INFO_NS = 'BuildInfo')
  env.SetDefault(BUILD_INFO_PACKAGE_VARS =
                 'Version Author Org Copyright Homepage License'.split())

  bld = Builder(action = build_function, source_factory = SCons.Node.FS.Entry,
                source_scanner = SCons.Defaults.DirScanner)
  env.Append(BUILDERS = {'BuildInfo' : bld})

  return True


def exists(env): return 1
