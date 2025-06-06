/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
                Copyright (c) 2003-2021, Cauldron Development LLC
                               All rights reserved.

         The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
       as published by the Free Software Foundation, either version 2.1 of
               the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
          but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                 Lesser General Public License for more details.

         You should have received a copy of the GNU Lesser General Public
                 License along with the C! library.  If not, see
                         <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
           You may request written permission by emailing the authors.

                  For information regarding this software email:
                                 Joseph Coffland
                          joseph@cauldrondevelopment.com

\******************************************************************************/

#include "SystemUtilities.h"

#include "Subprocess.h"
#include "DirectoryWalker.h"
#include "SysError.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/SmartPointer.h>

#include <cbang/time/Timer.h>
#include <cbang/time/Time.h>
#include <cbang/log/Logger.h>
#include <cbang/net/URI.h>
#include <cbang/comp/CompressionFilter.h>
#include <cbang/boost/IOStreams.h>
#include <cbang/util/ResourceManager.h>
#include <cbang/thread/Thread.h>
#include <cbang/io/IOStream.h>

#include <cerrno>
#include <cstring>
#include <ctime>
#include <cstdlib>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>
#include <io.h>
#include <direct.h> // For _chdir
#include <process.h>
#define getpid _getpid
#define WIN32_EXIT_MAGIC 0x61b582f6 // Used to detect a killed process

#else // _WIN32
#include <csignal>

#include <unistd.h>
#include <termios.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#endif // _WIN32

#ifdef __FreeBSD__
#include <sys/sysctl.h>
#endif // __FeeBSD__

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif // __APPLE__

#ifdef __linux__
#include <sched.h>
#endif // __linux__

#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BOOST_SYSTEM_NO_DEPRECATED
#include <cbang/boost/StartInclude.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <cbang/boost/EndInclude.h>

using namespace std;
using namespace cb;
using namespace cb::SystemUtilities;

namespace fs = boost::filesystem;


namespace {
  SmartPointer<iostream> defaultCreateFile(
    const string &path, ios::openmode mode, int perm) {
    return new IOStream(path, mode, perm);
  }
}

static createFile_t createFile = defaultCreateFile;


namespace cb {
  namespace SystemUtilities {
#ifdef _WIN32
    const string path_separators = "/\\";
    const char path_separator    = '\\';
    const char path_delimiter    = ';';
    const string library_path    = "PATH";
#else
    const string path_separators = "/";
    const char path_separator    = '/';
    const char path_delimiter    = ':';

#if defined(__APPLE__) || defined(__FreeBSD__)
    const string library_path = "DYLD_LIBRARY_PATH";
#else
    const string library_path = "LD_LIBRARY_PATH";
#endif // defined(__APPLE__) || defined(__FreeBSD__)

#endif

    bool useHardLinks = true;


    string basename(const string &path) {
      if (path.empty()) return path;
      string::size_type pos = path.find_last_of(path_separators);

      if (pos == string::npos) return path;
      else return path.substr(pos + 1);
    }


    string dirname(const string &path) {
      if (path.empty()) return path;
      string::size_type pos = path.find_last_of(path_separators);

      if (pos == path.length() - 1)
        pos = path.find_last_of(path_separators, pos - 1);

      if (pos == string::npos) return ".";
      else if (pos == 0) return "/";
      else return path.substr(0, pos);
    }


    string::size_type getExtensionPosition(const string &path) {
      string::size_type lastSep = path.find_last_of(path_separators);
      string::size_type pos = path.find_last_of('.');

      // Don't count '.' in directory names
      if (lastSep != string::npos && pos != string::npos && pos < lastSep)
        return string::npos;

      return pos;
    }


    bool hasExtension(const string &path) {
      return getExtensionPosition(path) != string::npos;
    }


    string extension(const string &path) {
      string::size_type pos = getExtensionPosition(path);
      return (pos == string::npos) ? string() : path.substr(pos + 1);
    }


    string removeExtension(const string &path) {
      string::size_type pos = getExtensionPosition(path);
      return path.substr(0, pos);
    }


    string swapExtension(const string &path, const string &ext) {
      return removeExtension(path) + "." + ext;
    }


    vector<string> splitExt(const string &path) {
      vector<string> result;

      string::size_type pos = getExtensionPosition(path);
      if (pos == string::npos) {
        result.push_back(path);
        result.push_back(string());

      } else {
        result.push_back(path.substr(0, pos));
        result.push_back(path.substr(pos + 1));
      }

      return result;
    }


    bool isAbsolute(const string &path) {
      return fs::path(path).is_absolute();
    }


    string relative(const string &base, const string &target,
                    unsigned maxDotDot) {
      // Find directory
      string dir;
      if (!exists(base) || isDirectory(base)) dir = base;
      else dir = dirname(base);

      // Split
      vector<string> parts1;
      vector<string> parts2;
      splitPath(getCanonicalPath(dir), parts1);
      splitPath(getCanonicalPath(target), parts2);

      // Find common parts
      unsigned i = 0;
      while (i < parts1.size() && i < parts2.size() && parts1[i] == parts2[i])
        i++;

      // Check number of ..'s
      if (maxDotDot < parts1.size() - i) return absolute(target);

      // Create relative path
      vector<string> parts;
      for (unsigned j = i; j < parts1.size(); j++) parts.push_back("..");
      for (unsigned j = i; j < parts2.size(); j++) parts.push_back(parts2[j]);

      return joinPath(parts);
    }


    string absolute(const string &base, const string &target) {
      if (isAbsolute(target)) return target;

      if (base.empty())
        return getCanonicalPath(".") + string(1, path_separator) + target;

      if (!exists(base) || isDirectory(base))
        return base + string(1, path_separator) + target;

      return dirname(base) + string(1, path_separator) + target;
    }


    string absolute(const string &path) {
      return absolute(getcwd(), path);
    }


    string getCanonicalPath(const string &_path) {
      if (_path.empty()) return _path;

      string path = _path;

      // Expand tilde
      if (path[0] == '~') {
        string name;
        unsigned i;
        for (i = 1; i < path.length() && path[i] != '/'; i++)
          name.append(1, path[i]);

#ifdef _WIN32
        if (name.empty()) {
#endif // _WIN32

          string home = getUserHome(name);
          if (i < path.length())
            path = home + string(1, path_separator) + path.substr(i + 1);
          else path = home;

#ifdef _WIN32
        }
#endif // _WIN32
      }

      fs::path p = fs::system_complete(path);
      string root = p.root_path().string();

      // Get relative part
      p = p.relative_path();

      // Normalize
      vector<string> parts;
      for (auto &part: p)
        if (part == ".." && !parts.empty()) parts.pop_back();
        else if (part != "" && part != "." && part != "..") {
#if BOOST_FILESYSTEM_VERSION < 3
          parts.push_back(part);
#else
          parts.push_back(part.string());
#endif
        }

      // Reassemble
      string result = root;
      for (unsigned i = 0; i < parts.size(); i++) {
        if (i) result += "/";
        result += parts[i];
      }

      return result;
    }


    bool exists(const string &path) {
      try {
        if (String::startsWith(path, "resource://"))
          return ResourceManager::instance().find(path.substr(11));

        return fs::exists(path);
      } catch (const exception &e) {
        THROW(e.what());
      }
    }


    bool isFile(const string &path) {
      try {
        return fs::is_regular_file(path);
      } catch (const exception &e) {
        THROW(e.what());
      }
    }


    bool isLink(const string &path) {
      try {
        return fs::is_symlink(path);
      } catch (const exception &e) {
        THROW(e.what());
      }
    }


    void splitPath(const string &path, vector<string> &parts) {
      String::tokenize(path, parts, path_separators);
    }


    void splitPaths(const string &s, vector<string> &paths) {
      String::tokenize(s, paths, string(1, path_delimiter));
    }


    string joinPath(const string &left, const string &right) {
      vector<string> parts;
      parts.push_back(left);
      parts.push_back(right);
      return joinPath(parts);
    }


    string joinPath(const vector<string> &parts) {
      return String::join(parts, string(1, path_separator));
    }


    string joinPaths(const vector<string> &paths) {
      return String::join(paths, string(1, path_delimiter));
    }


    string getExecutablePath() {
#ifdef _WIN32
      // Get module path
      TCHAR path[MAX_PATH];
      if (!GetModuleFileName(0, path, MAX_PATH))
        THROW("Failed to get module file name: " << SysError());

      return path;

#elif __APPLE__
      char path[PATH_MAX];
      uint32_t size = PATH_MAX;

      if (_NSGetExecutablePath(path, &size) == 0) return (char *)path;
      THROW("Failed to get executable path: " << SysError());

#elif __FreeBSD__
      static int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};

      char path[PATH_MAX + 1];
      size_t len = PATH_MAX;
      path[0] = path[PATH_MAX] = 0;

      if (sysctl(mib, 4, path, &len, 0, 0) || !*path)
        THROW("Failed to get executable path: " <<  SysError());

      return (char *)path;

#else
      char path[PATH_MAX + 1];
      ssize_t end;

      if ((end = readlink("/proc/self/exe", path, PATH_MAX)) == -1)
        THROW("Could not read link /proc/self/exe");

      path[end] = 0;

      return (char *)path;
#endif
    }


    string getPathPrefix() {return dirname(dirname(getExecutablePath()));}


    string findInPath(const string &path, const string &name) {
      if (basename(name) != name) THROW("Invalid name '" << name << "'");

      vector<string> paths;
      splitPaths(path, paths);

      for (auto &path: paths) {
        string filename = getCanonicalPath(joinPath(path, name));
        if (exists(filename)) return filename;
      }

      return "";
    }


    void mkdir(const string &path, bool withParents) {
      if (path.empty()) THROW("mkdir() path is empty");

      if (withParents) {
        string parent = dirname(path);

        if (parent != ".") {
          if (!isDirectory(parent)) {
            if (exists(parent))
              THROW("'" << parent << "' exists but is not a directory");

            mkdir(parent, true);
          }
        }
      }

      try {
        if (fs::create_directory(path)) return;
      } catch (const exception &e) {
        THROW("Failed to create directory '" << path << "': " << e.what());
      }

      THROW("Failed to create directory '" << path << "': " << SysError());
    }


    void rmdir(const string &path, bool withChildren) {
      if (!exists(path)) return;

      if (!isDirectory(path))
        THROW("Cannot remove '" << path << "' as directory");

      LOG_DEBUG(4, "Removing directory '" << path << "'");

      if (withChildren) rmtree(path);
      else if (::rmdir(path.c_str()))
        THROW("Failed to remove directory '" << path << "': " << SysError());
    }


    bool ensureDirectory(const string &path) {
      if (!isDirectory(path)) {
        if (exists(path))
          THROW("'" << path << "' exists but is not a directory");

        mkdir(path, true);

        LOG_DEBUG(5, "Created directory '" << path << "'");

        return true;
      }

      return false;
    }


    bool isDirectory(const string &path) {
      try {
        return fs::is_directory(path);
      } catch (const exception &e) {
        THROW(e.what());
      }
    }


    bool isDirectoryEmpty(const string &path) {
      return DirectoryWalker(path, ".*", 1, true).hasNext();
    }


    bool isDirectoryTreeEmpty(const string &path) {
      return !DirectoryWalker(path).hasNext();
    }


    string getcwd() {
      char buffer[4096];
#ifdef _WIN32
      return _getcwd(buffer, 4096);
#else
      return ::getcwd(buffer, 4096);
#endif
    }


    void chdir(const string &path) {
#ifdef _WIN32
      if (::_chdir(path.c_str()) < 0)
#else
      if (::chdir(path.c_str()) < 0)
#endif
        THROW("chdir(" << path << ") failed: " << SysError());
    }


    string createTempDir(const string &parent) {
      ensureDirectory(parent);

      SmartPointer<char>::Array buf = new char[parent.length() + 8];

      strcpy(buf.get(), parent.c_str());
      strcat(buf.get(), "/XXXXXX");

#ifdef _WIN32
      if (!_mktemp(buf.get()) || !fs::create_directory(buf.get()))
#else
        if (!mkdtemp(buf.get()))
#endif
          THROW("Failed to create temporary directory from template '"
                 << buf.get() << "'");

      return buf.get();
    }


    void listDirectory(
      const std::string &path,
      const std::function<void (const std::string &path, unsigned depth)> &cb,
      const std::string &pattern, unsigned maxDepth, bool listDirs) {
      DirectoryWalker walker(path, pattern, maxDepth, listDirs);

      while (walker.hasNext()) {
        std::string path = walker.next();
        unsigned depth = walker.getDepth();
        cb(path, depth);
      }
    }


    void listDirectory(vector<string> &paths, const string &path,
                       const string &pattern, unsigned maxDepth,
                       bool listDirs) {
      DirectoryWalker walker(path, pattern, maxDepth, listDirs);
      while (walker.hasNext()) paths.push_back(walker.next());
    }


    void rmtree(const string &path) {
      if (!exists(path)) return;
      if (!isDirectory(path) || isLink(path)) return (void)unlink(path);

      DirectoryWalker walker(path, ".*", ~0, true);

      while (walker.hasNext()) {
        string filename = walker.next();
        if (!isDirectory(filename) || isLink(filename)) (void)unlink(filename);
        else rmdir(filename);
      }
    }


    void setCreateFileCallback(createFile_t cb) {
      createFile = cb;
    }


    /// Check maximum number of open files
    unsigned getMaxFiles() {
#ifdef _WIN32
      THROW(CBANG_FUNC << "() not supported on Windows");

#else
      struct rlimit rlim;
      if (getrlimit(RLIMIT_NOFILE, &rlim))
        THROW("Failed to get open file limit");

      return rlim.rlim_cur == RLIM_INFINITY ?
        numeric_limits<unsigned>::max() : rlim.rlim_cur;
#endif
    }


    void setMaxFiles(unsigned files) {
#ifdef _WIN32
      THROW(CBANG_FUNC << "() not supported on Windows");

#else
      struct rlimit rlim;
      if (getrlimit(RLIMIT_NOFILE, &rlim))
        THROW("Failed to get open file limit");

      rlim.rlim_cur = std::min(rlim.rlim_max, (rlim_t)files);

      if (setrlimit(RLIMIT_NOFILE, &rlim))
        THROW("Failed to set open file limit");
#endif
    }


    uint64_t getFileSize(const string &filename) {
      if (!exists(filename))
        THROW("Error accessing file '" << filename << "'");

      try {
        return fs::file_size(filename);
      } catch (const exception &e) {
        THROW(e.what());
      }
    }


    uint64_t getModificationTime(const string &filename) {
      struct stat buf;
      if (stat(filename.c_str(), &buf))
        THROW("Accessing '" << filename << "': " << SysError());

      return buf.st_mtime;
    }


    bool unlink(const string &filename) {
      LOG_DEBUG(4, "Removing file '" << filename << "'");
      if (!exists(filename) && !isLink(filename)) return false;
      if (::unlink(filename.c_str()))
        THROW("Failed to remove '" << filename << "': " << SysError());
      return true;
    }


    void link(const string &oldname, const string &newname) {
      if (!useHardLinks) cp(oldname, newname);
      else
        try {
          fs::create_hard_link(oldname, newname);
        } catch (const exception &e) {
          THROW(e.what());
        }
    }


    void symlink(const string &oldname, const string &newname) {
      try {
        fs::create_symlink(oldname, newname);
      } catch (const exception &e) {
        THROW(e.what());
      }
    }


    uint64_t cp(const string &src, const string &dst, uint64_t length) {
      SmartPointer<iostream> in = open(src, ios::in);
      SmartPointer<iostream> out = open(dst, ios::out | ios::trunc);

      uint64_t bytes = cp(*in, *out, length);

      if (out->fail())
        THROW("Failed to copy '" << src << "' to '" << dst << "'");

      return bytes;
    }


    uint64_t cp(istream &in, ostream &out, uint64_t length) {
      const unsigned bufferSize = 102400;
      char buffer[bufferSize];
      uint64_t bytes = 0;

      while (!in.fail() && !out.fail() && length) {
        size_t size = length < bufferSize ? length : bufferSize;
        in.read(buffer, size);

        if ((size = in.gcount())) {
          bytes += size;
          out.write(buffer, size);
          length -= size;
        }
      }

      out.flush();

      return bytes;
    }


    void rename(const string &src, const string &dst) {
      if (exists(dst)) unlink(dst);
      bool fail = std::rename(src.c_str(), dst.c_str());

      if (fail && errno == EXDEV) { // Different file systems
        cp(src, dst);
        unlink(src);
        fail = false;
      }

      if (fail) THROW("Failed to rename '" << src << "' to '" << dst << "': "
                      << SysError());
    }


    void rotate(const string &path, const string &dir, unsigned maxFiles,
                Compression compression) {
      if (!exists(path)) return;

      string target;

      // Directory
      if (!dir.empty()) {
        ensureDirectory(dir);
        target = joinPath(dir, basename(path));

      } else target = path;

      // Name
      string ext = extension(target);
      if (!ext.empty()) {
        ext = "." + ext;
        target = target.substr(0, target.length() - ext.length());
      }

      target += Time().toString("-%Y%m%d-%H%M%S") + ext;

      // Move it
      rename(path, target);

      // Remove old log files
      string compExt = compressionExtension(compression);
      if (maxFiles) {
        string base = basename(path);
        if (!ext.empty()) base = base.substr(0, base.length() - ext.length());

        string searchDir;
        if (dir.empty()) searchDir = dirname(path);
        else searchDir = dir;

        string pattern =  String::escapeRE(base) +
          "-[0-9]{8}-[0-9]{6}" + String::escapeRE(ext);
        if (!compExt.empty()) pattern += "(" + String::escapeRE(compExt) + ")?";

        DirectoryWalker walker(searchDir, pattern, 1);
        set<string> files;

        while (walker.hasNext()) files.insert(walker.next());

        if (maxFiles < files.size()) {
          unsigned count = files.size() - maxFiles;
          for (auto &path: files) {
            if (!count--) break;
            LOG_INFO(3, "Removing old file '" << path << "'");
            unlink(path);
          }
        }
      }

      // Compression
      if (!compExt.empty()) {
        auto out = oopen(target + compExt, 0644, true);
        auto in  = iopen(target);

        auto cb = [out, in, target] {
          cp(*in, *out);
          unlink(target);
        };

        (new ThreadFunc(cb, true))->start();
      }
    }


#ifdef _WIN32
    class SmartWin32Handle {
      HANDLE h;
    public:
      SmartWin32Handle(HANDLE h) : h(h) {}
      ~SmartWin32Handle() {CloseHandle(h);}
      operator HANDLE () const {return h;}
      bool operator!() const {return !h;}
    };


    static HANDLE OpenProcess(DWORD access, uint64_t pid) {
      HANDLE h = ::OpenProcess(access, false, (DWORD)pid);
      if (!h)
        THROW("Failed to open process " << pid << ": " << SysError());
      return h;
    }
#endif // _WIN32


#define IS_SET(x, y) (((x) & (y)) == (y))
    int openModeToFlags(ios::openmode mode) {
      int flags = 0;

      if (IS_SET(mode, ios::in | ios::out)) flags |= O_RDWR;
      else if (IS_SET(mode, ios::in)) flags |= O_RDONLY;
      else if (IS_SET(mode, ios::out)) {
        flags |= O_WRONLY;
        if (!(IS_SET(mode, ios::ate) || IS_SET(mode, ios::app)))
          flags |= O_TRUNC;
      }

      if (IS_SET(mode, ios::out)) {
        if (IS_SET(mode, ios::trunc)) flags |= O_TRUNC;
        if (IS_SET(mode, ios::app)) flags |= O_APPEND;
        flags |= O_CREAT; // The default for ios::out
      }

#ifdef _WIN32
      if (IS_SET(mode, ios::binary)) flags |= O_BINARY;
#endif // _WIN32

      return flags;
    }


    int priorityToInt(ProcessPriority priority) {
      switch (priority) {
      case ProcessPriority::PRIORITY_NORMAL:   return 0;
      case ProcessPriority::PRIORITY_IDLE:     return 19;
      case ProcessPriority::PRIORITY_LOW:      return 10;
      case ProcessPriority::PRIORITY_HIGH:     return -10;
      case ProcessPriority::PRIORITY_REALTIME: return -20;
      default: THROW("Invalid priority: " << priority);
      }
    }


    void setPriority(ProcessPriority priority, uint64_t pid) {
      if (priority == ProcessPriority::PRIORITY_INHERIT) return;

#ifdef _WIN32
      if (!pid) pid = (uint64_t)GetCurrentProcessId();

      DWORD pclass = Subprocess::priorityToClass(priority);

      SmartWin32Handle h = OpenProcess(PROCESS_SET_INFORMATION, pid);
      bool err = !SetPriorityClass(h, pclass);

      if (err) THROW("Failed to set process priority: " << SysError());

#elif defined(__linux__)
      if (priority == ProcessPriority::PRIORITY_IDLE) {
        // Instead of setting nice +19, on Linux we can use the SCHED_IDLE
        // scheduler class. This is similar to the IDLE_PRIORITY_CLASS on
        // Windows: the targetted process will only run when there are spare
        // CPU cycles, and will be pre-empted immediately whenever anything
        // else wants to run. This helps avoid inducing lag on the rest of the
        // system if the targetted process is CPU-heavy.
        //
        // When SCHED_IDLE is set, the nice value of the process does not
        // affect scheduling. However, it *does* affect tools like `top` or
        // `htop` which track system usage, in two ways:
        //
        // 1) The nice value affects whether this process's runtime is counted
        //    in the "normal" or the "low-priority" summary statistics
        //
        // 2) The nice value is also reported in /proc/PID/stat as-is,
        //    so unless the tool explicitly checks the scheduler policy it will
        //    receive this value.
        //
        // In order to be nicer to those tools, we fall through and set nice
        // +19 as well as SCHED_IDLE, even though this is redundant from a
        // purely scheduling viewpoint.
        //
        // For context, see "Fixing SCHED_IDLE" on LWN:
        // https://lwn.net/Articles/805317/
        SysError::clear();

        struct sched_param scheduler_params = {.sched_priority = 0};
        sched_setscheduler((pid_t)pid, SCHED_IDLE, &scheduler_params);

        if (SysError::get())
          THROW("Failed to set process priority: " << SysError());
      }

      SysError::clear();
      setpriority(PRIO_PROCESS, (pid_t)pid, priorityToInt(priority));

      if (SysError::get())
        THROW("Failed to set process priority: " << SysError());
#else
      SysError::clear();
      setpriority(PRIO_PROCESS, (pid_t)pid, priorityToInt(priority));

      if (SysError::get())
        THROW("Failed to set process priority: " << SysError());
#endif
    }


    int system(const string &cmd, const StringMap &env) {
      Subprocess process;

      process.insert(env.begin(), env.end());
      process.exec(cmd, Subprocess::SHELL |
                   Subprocess::MERGE_STDOUT_AND_STDERR |
                   Subprocess::REDIR_STDOUT);

      // Copy output to log
      uint64_t pid = process.getPID();
      auto out = process.getPipeOut().toStream();
      char line[4096];

      while (!out->fail()) {
        out->getline(line, 4096);
        LOG_INFO(2, 'P' << pid << ": " << line);
      }

      return process.wait();
    }


    bool pidAlive(uint64_t pid) {
#ifdef _WIN32
      try {
        SmartWin32Handle h = OpenProcess(PROCESS_QUERY_INFORMATION, pid);

        DWORD status;
        bool ret = GetExitCodeProcess(h, &status) && status == STILL_ACTIVE;

        return ret;
      } catch (const Exception &e) {return false;}

#else
      errno = 0;
      if (::kill((pid_t)pid, 0) == -1) {
        if (errno == ESRCH) return false;
        LOG_ERROR("Failed to check if process " << pid << " is alive: "
                  << SysError());
      }

      return true;
#endif
    }


    double getCPUTime() {
#ifdef _WIN32
      FILETIME createTime, exitTime, kernelTime, userTime;
      if (!GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime,
                           &kernelTime, &userTime))
        THROW("Could not get CPU time: " << SysError());

      double t =
        (((UINT64)kernelTime.dwHighDateTime + userTime.dwHighDateTime) << 32) +
        (UINT64)kernelTime.dwLowDateTime + userTime.dwLowDateTime;

      return t / 10000000; // 100ns units

#else
      struct tms t;
      if (times(&t) == (clock_t)-1)
        THROW("Could not get CPU time: " << SysError());

      return (double)(t.tms_utime + t.tms_stime) / sysconf(_SC_CLK_TCK);
#endif
    }


    uint64_t getPID() {return (uint64_t)getpid();}


    bool waitPID(uint64_t pid, int *returnCode, bool nonblocking, int *flags) {
      if (flags) *flags = 0;

#ifdef _WIN32
      SmartWin32Handle h =
        OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, pid);
      if (!h) THROW("Failed to access PID " << pid << ": " << SysError());

      // Check if process has exited
      switch (WaitForSingleObject(h, nonblocking ? 0 : INFINITE)) {
      case WAIT_TIMEOUT: return false; // Still running

      case WAIT_OBJECT_0: { // Process has ended, get exit code
        DWORD exitCode = 0;
        if (!GetExitCodeProcess(h, &exitCode))
          THROW("Failed to get PID " << pid << " exit code: " << SysError());

        // NOTE Checking for STILL_ACTIVE is unreliable
        if (returnCode) *returnCode = exitCode;
        if (flags) {
          if (exitCode == WIN32_EXIT_MAGIC)
            *flags = Subprocess::PROCESS_SIGNALED;
          else *flags = Subprocess::PROCESS_EXITED;
        }

        return true; // Exited
      }

      default: THROW("Failed to wait for PID " << pid << ": " << SysError());
      }

#else // _WIN32
      int status = 0;
      int retVal = waitpid((pid_t)pid, &status, nonblocking ? WNOHANG : 0);

      if (retVal == -1)
        THROW("Failed to wait on process " << pid << ":" << SysError());

      if (!retVal) return false; // Still running

      if (returnCode && WIFEXITED(status)) *returnCode = WEXITSTATUS(status);

      if (flags) {
        if (WIFEXITED(status))   *flags |= Subprocess::PROCESS_EXITED;
        if (WIFSIGNALED(status)) *flags |= Subprocess::PROCESS_SIGNALED;
        if (WCOREDUMP(status))   *flags |= Subprocess::PROCESS_DUMPED_CORE;
      }

      return true;
#endif // _WIN32
    }


    bool killPID(uint64_t pid, bool group) {
      try {

#ifdef _WIN32
        SmartWin32Handle h = OpenProcess(PROCESS_TERMINATE, pid);
        if (TerminateProcess(h, WIN32_EXIT_MAGIC)) return true;

#else // _WIN32
        int err;
        if (group) err = ::killpg((pid_t)pid, SIGKILL);
        else err = ::kill((pid_t)pid, SIGKILL);

        if (!err) return true;
#endif // _WIN32

      } catch (const Exception &e) {} // Ignore

      return false;
    }


    void setGroup(const string &group) {
#ifdef _WIN32
      THROW("setGroup() not supported on Windows systems.");
#else
      gid_t gid;

      try {
        gid = (gid_t)String::parseU32(group);
      } catch (const Exception &e) {
        gid = 0;
      }

      if (!gid) {
        // TODO this call is not reentrable
        struct group *entry = getgrnam(group.c_str());
        if (!entry) THROW("Could not find group '" << group << "'");
        gid = entry->gr_gid;
      }

      if (setgroups(1, &gid) == -1)
        THROW("Failed to set group ID to " << gid << ": " << SysError());
#endif
    }


    void setUser(const string &user) {
#ifdef _WIN32
      THROW("setUser() not supported on Windows systems.");
#else
      unsigned uid;

      try {
        uid = String::parseU32(user);
      } catch (const Exception &e) {
        uid = 0;
      }

      if (!uid) {
        // TODO this call is not reentrable
        struct passwd *entry = getpwnam(user.c_str());
        if (!entry) THROW("Could not find user '" << user << "'");
        uid = entry->pw_uid;
      }

      if (setuid(uid) == -1)
        THROW("Failed to set user ID to " << uid << ": " << SysError());
#endif
    }


    void interruptProcessGroup() {
#ifdef _WIN32
      GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0);
#else
      ::kill(0, SIGINT);
#endif
    }


    void daemonize() {
#ifdef _WIN32
      THROW("Daemonize not supported in Windows");
#else // _WIN32

      // Already a daemon
      if (::getppid() == 1) return;

      // Fork the parent process
      pid_t pid = ::fork();
      if (pid < 0) THROW("Failed to daemonize: " << SysError());

      // If we got a good PID, then we can exit the parent process.
      if (pid > 0) ::exit(0);

      // Configure child process
      umask(0); // Reset file mode mask

      // Create a new SID
      pid_t sid = setsid();
      if (sid < 0) THROW("Failed to create new session ID: " << SysError());

      // Redirect standard files to /dev/null
      if (!freopen("/dev/null", "r", stdin))
        LOG_WARNING("Failed to redirect stdin to /dev/null");
      if (!freopen("/dev/null", "w", stdout))
        LOG_WARNING("Failed to redirect stdout to /dev/null");
      if (!freopen("/dev/null", "w", stderr))
        LOG_WARNING("Failed to redirect stderr to /dev/null");
#endif // _WIN32
    }


    SmartPointer<iostream> open(const string &filename, ios::openmode mode,
                                int perm) {
      if ((mode & ios::out) == ios::out) ensureDirectory(dirname(filename));

      SysError::clear();
      try {
        return createFile(filename, mode, perm);

      } catch (const exception &e) {
        THROW("Failed to open '" << filename << "': " << e.what()
               << ": " << SysError());
      }
    }


    struct IStream : public io::filtering_istream {
      SmartPointer<istream> file;
      IStream(const SmartPointer<istream> &file) : file(file) {}
      ~IStream() {reset();}
    };


    SmartPointer<istream> iopen(const string &filename, bool autoCompression) {
      SysError::clear();
      try {
        SmartPointer<istream> file;

        if (String::startsWith(filename, "resource://"))
          file = ResourceManager::instance().open(filename.substr(11));
        else file = createFile(filename, ios::in, 0);

        if (autoCompression) {
          Compression compression = compressionFromPath(filename);

          if (compression != Compression::COMPRESSION_NONE) {
            SmartPointer<IStream> s = new IStream(file);
            pushDecompression(compression, *s);
            s->push(*file);

            return s;
          }
        }

        return file;

      } catch (const exception &e) {
        THROW("Failed to open '" << filename << "': " << e.what()
              << ": " << SysError());
      }
    }


    struct OStream : public io::filtering_ostream {
      SmartPointer<ostream> file;
      OStream(const SmartPointer<ostream> &file) : file(file) {}
      ~OStream() {reset();}
    };


    SmartPointer<ostream> oopen(const string &filename, int perm,
                                bool autoCompression) {
      ensureDirectory(dirname(filename));

      SysError::clear();
      try {
        auto file = createFile(filename, ios::out, perm);

        if (autoCompression) {
          Compression compression = compressionFromPath(filename);

          if (compression != Compression::COMPRESSION_NONE) {
            SmartPointer<OStream> s = new OStream(file);
            pushCompression(compression, *s);
            s->push(*file);

            return s;
          }
        }

        return file;

      } catch (const exception &e) {
        THROW("Failed to open '" << filename << "': " << e.what()
               << ": " << SysError());
      }
    }


    string read(std::istream &stream, uint64_t length) {
      ostringstream str;
      cp(stream, str, length);

      return str.str();
    }


    string read(const string &filename, uint64_t length) {
      return read(*iopen(filename), length);
    }


    string getline(istream &stream, uint64_t length) {
      if (stream.fail()) THROW("Failed stream");

      SmartPointer<char>::Array line = new char[length];
      stream.getline(line.get(), length);
      return string(line.get(), stream.gcount());
    }


    void truncate(const string &path, unsigned long length) {
      bool failed;

#ifdef _WIN32
      int fd = ::open(path.c_str(), O_WRONLY);

      if (fd != -1) {
        failed = _chsize(fd, length);
        close(fd);

      } else failed = true;

#else
      failed = ::truncate(path.c_str(), length);
#endif

      if (failed)
        THROW("Failed to truncate '" << path << "' to " << length << ": "
               << SysError());
    }


    void chmod(const string &path, unsigned mode) {
      bool fail;

#ifdef _WIN32
      if (isDirectory(path)) return;
      fail = _chmod(path.c_str(), mode & (_S_IWRITE | _S_IREAD)) != 0;
#else

      fail = ::chmod(path.c_str(), mode) != 0;
#endif

      if (fail)
        THROW("Failed to change permissions on '" << path << "'"
               << SysError());
    }

    string getUserHome(const string &name) {
      if (name.empty()) return getenv("HOME") ? getenv("HOME") : "";

#ifdef _WIN32
      // TODO implement for Windows
      THROW("function not yet implemented in Windows");

#else // _WIN32
      struct passwd u;
      char buffer[4096];
      struct passwd *result;

      if (getpwnam_r(name.c_str(), &u, buffer, 4096, &result))
        THROW("Failed to get user '" << name << "'s info " << SysError());

      if (!result) THROW("User '" << name << "' does not exist");

      return u.pw_dir;
#endif // _WIN32
    }


    const char *getenv(const string &name) {
      return ::getenv(name.c_str());
    }


    void setenv(const string &name, const string &value) {
#ifdef _WIN32
      _putenv((name + "=" + value).c_str());
#else
      ::setenv(name.c_str(), value.c_str(), true);
#endif
    }


    void unsetenv(const string &name) {
#ifdef _WIN32
      _putenv((name + "=").c_str());
#else
      ::unsetenv(name.c_str());
#endif
    }


    void clearenv() {
#ifdef _WIN32
      THROW("clearenv() not supported in Windows");
#elif __APPLE__
      THROW("clearenv() not supported in OSX");
#else
      ::clearenv();
#endif
    }


    string readline(istream &in, ostream &out, const string &message,
                    const string &defaultValue, const string &suffix) {
      char buffer[1024];

      out << message;
      if (!defaultValue.empty()) out << " [" << defaultValue << "]";
      out << suffix << flush;

      in.getline(buffer, 1024);

      if (strlen(buffer)) return buffer;

      return defaultValue;
    }


    SmartPointer<Subprocess> openURI(const URI &uri) {
      vector<string> cmd;

#ifdef _WIN32
      cmd.push_back("start");
#elif __APPLE__
      cmd.push_back("open");
#else
      cmd.push_back("xdg-open");
#endif

      cmd.push_back(uri.toString());

      auto p = SmartPtr(new Subprocess);
      p->exec(cmd, Subprocess::SHELL);
      return p;
    }

  } // namespace SystemUtilities
}  // namespace cb
