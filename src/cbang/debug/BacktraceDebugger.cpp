/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include "BacktraceDebugger.h"
#include "BFDResolver.h"
#include "Demangle.h"

#include <cbang/config.h>

#ifdef HAVE_CBANG_BACKTRACE

#include <cbang/thread/SmartLock.h>
#include <cbang/os/SystemUtilities.h>

#include <execinfo.h>

#ifdef HAVE_VALGRIND
#include <valgrind/memcheck.h>
#endif

// NOTE Cannot throw cb::Exception here because of the dependency loop

using namespace std;
using namespace cb;


BacktraceDebugger::BacktraceDebugger() : initialized(false), parents(2) {}
BacktraceDebugger::~BacktraceDebugger() {}
bool BacktraceDebugger::supported() {return true;}


void BacktraceDebugger::getStackTrace(StackTrace &trace, bool resolved) {
  init();

  void *stack[maxStack];
  int n = backtrace(stack, maxStack);

#ifdef VALGRIND_MAKE_MEM_DEFINED
  (void)VALGRIND_MAKE_MEM_DEFINED(stack, n * sizeof(void *));
#endif // VALGRIND_MAKE_MEM_DEFINED

  for (int i = 0; i < n; i++)
    trace.push_back(StackFrame(stack[i]));

  if (resolved) resolve(trace);
}


void BacktraceDebugger::resolve(StackTrace &trace) {
  SmartLock lock(this);

  for (unsigned i = 0; i < trace.size(); i++)
    if (trace[i].getLocation()) break;
    else trace[i].setLocation(&resolve(trace[i].getAddr()));
}


const FileLocation &BacktraceDebugger::resolve(void *addr) {
  auto it = cache.find(addr);
  if (it != cache.end()) return *it->second;

  string filename;
  string function;
  int line = -1;

  // BFD symbol resolver
  if (bfdResolver.isSet()) bfdResolver->resolve(addr, filename, function, line);

  // Fallback to backtrace symbols
  if (function.empty() || filename.empty()) {
    SmartPointer<char *>::Malloc symbols = backtrace_symbols(&addr, 1);
    char *sym = symbols.isSet() ? symbols[0] : 0;

    // Parse symbol string
#if defined(__APPLE__)
    // Expected format: # <module> <address> <function> + <offset>
    if (sym) {
      vector<string> parts;
      String::tokenize(sym, parts);

      if (parts.size() == 6) {
        if (filename.empty()) filename = parts[1];
        if (function.empty()) function = parts[3];
      }
    }

#else
    // Expected format: <module>(<function>+0x<offset>)[<address>]
    char *ptr = sym;
    while (ptr && *ptr && *ptr != '(') ptr++;
    if (*ptr == '(') {
      // Not really the source file name
      if (filename.empty()) filename = string(sym, ptr - sym);

      if (function.empty() && ptr[1] != ')') {
        char *start = ++ptr;
        while (*ptr && *ptr != '+') ptr++;
        function = string(start, ptr - start);
      }
    }
#endif // !__APPLE__
  }

  // Filename
  if (0 <= parents) {
    size_t ptr = string::npos;

    for (int j = 0; j <= parents; j++) {
      if (ptr != string::npos && ptr) ptr--;
      ptr = filename.find_last_of('/', ptr);
    }

    if (ptr && ptr != string::npos) filename = filename.substr(ptr + 1);
  }

  SmartPointer<FileLocation> loc =
    new FileLocation(filename, demangle(function.c_str()), line);

  cache.insert(cache_t::value_type(addr, loc));
  return *loc;
}


void BacktraceDebugger::init() {
  if (initialized) return;
  initialized = true;

  try {
    bfdResolver = new BFDResolver(SystemUtilities::getExecutablePath());
  } catch (const std::exception &e) {
    cerr << "Failed to initialize BFD for stack traces: " << e.what() << endl;
  }
}


#else // HAVE_CBANG_BACKTRACE
bool cb::BacktraceDebugger::supported() {return false;}
#endif // HAVE_CBANG_BACKTRACE
