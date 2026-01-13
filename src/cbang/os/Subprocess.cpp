/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
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

#include "Subprocess.h"
#include "SystemUtilities.h"
#include "SysError.h"
#include "SystemInfo.h"

#include <cbang/Exception.h>
#include <cbang/Catch.h>
#include <cbang/String.h>
#include <cbang/time/Timer.h>
#include <cbang/log/Logger.h>

#include <cstring> // For memset()

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>

#else // _WIN32
#include <cstdio>
#include <cerrno>
#include <csignal>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif // _WIN32

using namespace cb;
using namespace std;


namespace {
#ifdef _WIN32
  HANDLE OpenNUL() {
    // Setup security attributes for pipe inheritance
    SECURITY_ATTRIBUTES sAttrs;
    memset(&sAttrs, 0, sizeof(SECURITY_ATTRIBUTES));
    sAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);
    sAttrs.bInheritHandle = TRUE;
    sAttrs.lpSecurityDescriptor = 0;

    return CreateFile("NUL", GENERIC_WRITE, 0, &sAttrs, OPEN_EXISTING, 0, 0);
  }
#else


  void inChildProc(Pipe &pipe, PipeEnd::handle_t target = -1) {
    pipe.getParentEnd().close();

    // Move to target
    if (target != -1 && !pipe.getChildEnd().moveFD(target))
      perror("Moving file descriptor");
  }
#endif
}


struct Subprocess::Private {
#ifdef _WIN32
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
#else
  pid_t pid = 0;
#endif

  Private() {
#ifdef _WIN32
    memset(&pi, 0, sizeof(PROCESS_INFORMATION));
    memset(&si, 0, sizeof(STARTUPINFO));
#endif
  }
};


Subprocess::Subprocess() : p(new Private) {
  pipes.push_back(Pipe(true));  // stdin
  pipes.push_back(Pipe(false)); // stdout
  pipes.push_back(Pipe(false)); // stderr
}


Subprocess::~Subprocess() {
  try {
    if (isRunning())
      LOG_ERROR("Subprocess deallocated while process is still running");
  } CATCH_ERROR;

  closeHandles();
  delete p;
}


bool Subprocess::isRunning() {
  if (running) wait(true);
  return running;
}


uint64_t Subprocess::getPID() const {
#ifdef _WIN32
  return p->pi.dwProcessId;

#else // _WIN32
  return p->pid;
#endif // _WIN32
}


unsigned Subprocess::createPipe(bool toChild) {
  pipes.push_back(Pipe(toChild));
  pipes.back().create();
  return pipes.size() - 1;
}


PipeEnd &Subprocess::getPipe(unsigned i) {
  if (pipes.size() <= i) THROW("Subprocess does not have pipe " << i);
  return pipes[i].getParentEnd();
}


void Subprocess::exec(const vector<string> &_args, unsigned flags,
                      ProcessPriority priority) {
  if (isRunning()) THROW("Subprocess already running");

  returnCode = exitFlags = 0;

  LOG_DEBUG(3, "Executing: " << assemble(_args));

  // Don't redirect stderr if merging
  if (flags & MERGE_STDOUT_AND_STDERR) flags &= ~REDIR_STDERR;

  // Don't allow both redirecting and nulling pipes
  if (flags & NULL_STDOUT && flags & REDIR_STDOUT)
    THROW("Cannot both null and redirect stdout");
  if (flags & NULL_STDERR && flags & REDIR_STDERR)
    THROW("Cannot both null and redirect stderr");

  // Send signals to whole process group
  signalGroup = flags & CREATE_PROCESS_GROUP;

  try {
    // Create pipes
    if (flags & REDIR_STDIN)  pipes[0].create();
    if (flags & REDIR_STDOUT) pipes[1].create();
    if (flags & REDIR_STDERR) pipes[2].create();

#ifdef _WIN32
    // Don't let child process inherit parent handles
    for (unsigned i = 0; i < pipes.size(); i++)
      if (getPipe(i).isOpen())
        getPipe(i).setInheritFlag(false);

    // Clear PROCESS_INFORMATION
    memset(&p->pi, 0, sizeof(PROCESS_INFORMATION));

    // Configure STARTUPINFO
    memset(&p->si, 0, sizeof(STARTUPINFO));
    p->si.cb = sizeof(STARTUPINFO);
    p->si.hStdInput =
      (flags & REDIR_STDIN)  ? pipes[0].getChildEnd().getHandle() :
      GetStdHandle(STD_INPUT_HANDLE);
    p->si.hStdOutput =
      (flags & REDIR_STDOUT) ? pipes[1].getChildEnd().getHandle() :
      GetStdHandle(STD_OUTPUT_HANDLE);
    p->si.hStdError =
      (flags & REDIR_STDERR) ? pipes[2].getChildEnd().getHandle() :
      ((flags & MERGE_STDOUT_AND_STDERR) ?
       pipes[1].getChildEnd().getHandle() : GetStdHandle(STD_ERROR_HANDLE));
    p->si.dwFlags |= STARTF_USESTDHANDLES;

    if (flags & NULL_STDOUT) p->si.hStdOutput = OpenNUL();
    if (flags & NULL_STDERR) p->si.hStdError  = OpenNUL();

    // Hide window
    if (flags & W32_HIDE_WINDOW) {
      p->si.dwFlags |= STARTF_USESHOWWINDOW;
      p->si.wShowWindow = SW_HIDE;
    }

    // Setup environment
    const char *env = 0; // Use parents environment
    vector<char> newEnv;

    if (!StringMap::empty()) {
      StringMap copy;

      if (!(flags & CLEAR_ENVIRONMENT)) {
        // Copy environment
        LPTCH envStrs = GetEnvironmentStrings();
        if (envStrs) {
          const char *ptr = envStrs;
          do {
            const char *equal = strchr(ptr, '=');

            if (equal) copy[string(ptr, equal - ptr)] = string(equal + 1);

            ptr += strlen(ptr) + 1;

          } while (*ptr);

          FreeEnvironmentStrings(envStrs);
        }
      }

      // Overwrite with Subprocess vars
      copy.insert(StringMap::begin(), StringMap::end());

      // Load into vector
      for (auto &p: copy) {
        string s = p.first + '=' + p.second;
        newEnv.insert(newEnv.end(), s.begin(), s.end());
        newEnv.push_back(0); // Terminate string
      }
      newEnv.push_back(0); // Terminate block

      env = &newEnv[0];

    } else if (flags & CLEAR_ENVIRONMENT) {
      newEnv.push_back(0);
      newEnv.push_back(0);
      env = &newEnv[0];
    }

    // Working directory
    const char *dir = 0; // Default to current directory
    string wd = this->wd;
    if (!wd.empty()) {
      if (!SystemUtilities::isAbsolute(wd)) wd = SystemUtilities::absolute(wd);
      dir = wd.c_str();
    }

    // Prepare command
    string command = assemble(_args);
    if (flags & SHELL) command = "cmd.exe /c " + command;

    // Creation flags
    DWORD cFlags = 0;
    if (flags & CREATE_PROCESS_GROUP) cFlags |= CREATE_NEW_PROCESS_GROUP;

    // Priority
    cFlags |= priorityToClass(priority);

    // Start process
    if (!CreateProcess(0, (LPSTR)command.c_str(), 0, 0, TRUE, cFlags,
                       (LPVOID)env, dir, &p->si, &p->pi))
      THROW("Failed to create process with: " << command << ": "
             << SysError());

    if (flags & W32_WAIT_FOR_INPUT_IDLE) {
      int ret = WaitForInputIdle(p->pi.hProcess, 5000);
      if (ret == (int)WAIT_TIMEOUT) THROW("Wait timedout");
      if (ret == (int)WAIT_FAILED) THROW("Wait failed");
    }

#else // _WIN32
    // Convert args
    vector<char *> args;
    for (auto &arg: _args)
      args.push_back((char *)arg.c_str());
    args.push_back(0); // Sentinal

#ifdef __APPLE__
    // vfork deprecated in macos 12.0, previously discouraged
    p->pid = fork();
#else
    if (flags & USE_VFORK) p->pid = vfork();
    else p->pid = fork();
#endif

    if (!p->pid) { // Child
      vector<Pipe> pipes = this->pipes; // Make a copy for vfork case

      // Process group
      if (flags & CREATE_PROCESS_GROUP) setpgid(0, 0);

      // Configure pipes
      if (flags & REDIR_STDIN)  inChildProc(pipes[0], 0);
      if (flags & REDIR_STDOUT) inChildProc(pipes[1], 1);

      if (flags & MERGE_STDOUT_AND_STDERR)
        if (dup2(1, 2) != 2) perror("Copying stdout to stderr");

      if (flags & REDIR_STDERR) inChildProc(pipes[2], 2);

      if ((flags & NULL_STDOUT) || (flags & NULL_STDERR)) {
        int fd = open("/dev/null", O_WRONLY);

        if (fd != -1) {
          if (flags & NULL_STDOUT) dup2(fd, 1);
          if (flags & NULL_STDERR) dup2(fd, 2);
          close(fd);

        } else {
          if (flags & NULL_STDOUT) close(1);
          if (flags & NULL_STDERR) close(2);
        }
      }

      for (unsigned i = 3; i < pipes.size(); i++)
        inChildProc(pipes[i]);

      // Priority
      SystemUtilities::setPriority(priority);

      // Working directory
      if (wd != "") SystemUtilities::chdir(wd);

      // Setup environment
      if (flags & CLEAR_ENVIRONMENT) SystemUtilities::clearenv();

      for (auto &p: *this)
        SystemUtilities::setenv(p.first, p.second);

      SysError::clear();

      if (flags & SHELL) execvp(args[0], &args[0]);
      else execv(args[0], &args[0]);

      // Execution failed
      string errorStr = "Failed to execute: " + String::join(_args);
      perror(errorStr.c_str());
      exit(-1);

    } else if (p->pid == -1)
      THROW("Failed to spawn subprocess: " << SysError());
#endif // _WIN32

  } catch (...) {
    closeHandles();
    throw;
  }

  // Max pipe size
#ifdef F_SETPIPE_SZ
  try {
    if (flags & MAX_PIPE_SIZE &&
        SystemUtilities::exists("/proc/sys/fs/pipe-max-size")) {
      string num = SystemUtilities::read("/proc/sys/fs/pipe-max-size");
      int size = (int)String::parseU32(num);

      if (flags & REDIR_STDIN)  getPipeIn() .setSize(size);
      if (flags & REDIR_STDOUT) getPipeOut().setSize(size);
      if (flags & REDIR_STDERR) getPipeErr().setSize(size);
      for (unsigned i = 3; i < pipes.size(); i++)
        getPipe(i).setSize(size);
    }
  } CATCH_ERROR;

#else // F_SETPIPE_SZ
  if (flags & MAX_PIPE_SIZE)
    LOG_WARNING("Subprocess MAX_PIPE_SIZE not supported");
#endif // F_SETPIPE_SZ

  // Close pipe child ends
  for (auto &pipe: pipes)
    pipe.getChildEnd().close();

  running = true;
}


void Subprocess::exec(const string &command, unsigned flags,
                      ProcessPriority priority) {
  vector<string> args;
  parse(command, args);
  exec(args, flags, priority);
}


void Subprocess::interrupt() {
  if (!running) THROW("Process not running!");

#ifdef _WIN32
  if (GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, p->pi.dwProcessId)) return;
#else // _WIN32
  if (!(signalGroup ? ::killpg : ::kill)((pid_t)getPID(), SIGINT)) return;
#endif // _WIN32

  THROW("Failed to interrupt process " << getPID() << ": " << SysError());
}


bool Subprocess::kill(bool nonblocking) {
  if (!running) THROW("Process not running!");

#ifdef _WIN32
  HANDLE h = p->pi.hProcess;
  if (!h) h = OpenProcess(PROCESS_TERMINATE, false, getPID());
  if (!h || !TerminateProcess(h, -1)) return false;

#else
  if (!(signalGroup ? ::killpg : ::kill)((pid_t)getPID(), SIGKILL))
    return false;
#endif

  if (!nonblocking) wait();

  return true;
}


int Subprocess::wait(bool nonblocking) {
  if (!running) return returnCode;

  try {
    running =
      !SystemUtilities::waitPID(getPID(), &returnCode, nonblocking, &exitFlags);

  } catch (...) {
    closeProcessHandles();
    running = false;
    throw;
  }

  if (!running) closeProcessHandles();

  return returnCode;
}


void Subprocess::parse(const string &command, vector<string> &args) {
  bool inSQuote = false;
  bool inDQuote = false;
  bool escape = false;
  unsigned len = command.length();
  string arg;

  for (unsigned i = 0; i < len; i++) {
    if (escape) {
      arg += command[i];
      escape = false;
      continue;
    }

    switch (command[i]) {
    case '\\': escape = true; break;

    case '\'':
      if (inDQuote) arg += command[i];
      else inSQuote = !inSQuote;
      break;

    case '"':
      if (inSQuote) arg += command[i];
      else inDQuote = !inDQuote;
      break;

    case '\t':
    case ' ':
    case '\n':
    case '\r':
      if (inSQuote || inDQuote) arg += command[i];
      else if (arg.length()) {
        args.push_back(arg);
        arg.clear();
      }
      break;

    default: arg += command[i];
    }
  }

  if (arg.length()) args.push_back(arg);
}


string Subprocess::assemble(const vector<string> &args) {
  string command;

  for (auto &arg: args) {
    if (!command.empty()) command += " ";

    // Check if we need to quote this arg
    bool quote = false;
    for (auto c: arg)
      if (isspace(c) || c == '"') {quote = true; break;}

    if (quote) command += '"';
    command += String::replace(arg, "\"", "\\\\\"");
    if (quote) command += '"';
  }

  return command;
}


unsigned Subprocess::priorityToClass(ProcessPriority priority) {
#ifdef _WIN32
  switch (priority) {
  case PRIORITY_INHERIT:  return 0;
  case PRIORITY_NORMAL:   return NORMAL_PRIORITY_CLASS;
  case PRIORITY_IDLE:     return IDLE_PRIORITY_CLASS;
  case PRIORITY_LOW:      return BELOW_NORMAL_PRIORITY_CLASS;
  case PRIORITY_HIGH:     return ABOVE_NORMAL_PRIORITY_CLASS;
  case PRIORITY_REALTIME: return REALTIME_PRIORITY_CLASS;
  default: THROW("Invalid priority: " << priority);
  }

#else // _WIN32
  THROW(CBANG_FUNC << "() not supported outside of Windows");
#endif // _WIN32
}


void Subprocess::closeProcessHandles() {
#ifdef _WIN32
  // Close process & thread handles
  if (p->pi.hProcess) {CloseHandle(p->pi.hProcess); p->pi.hProcess = 0;}
  if (p->pi.hThread)  {CloseHandle(p->pi.hThread);  p->pi.hThread  = 0;}
#endif
}


void Subprocess::closePipes() {for (auto &pipe: pipes) pipe.close();}


void Subprocess::closeHandles() {
  closeProcessHandles();
  closePipes();
}
