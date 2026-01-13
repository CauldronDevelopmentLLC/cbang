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

#include "DynamicLibrary.h"
#include "SysError.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#undef CBANG_EXCEPTION
#define CBANG_EXCEPTION DynamicLibraryException

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>

#else
#include <dlfcn.h>
#endif

using namespace std;
using namespace cb;


bool DynamicLibrary::enabled = true;


struct DynamicLibrary::impl_t {
#ifdef _WIN32
  HMODULE handle;

  impl_t(const string &path) :
    handle(LoadLibraryEx(path.c_str(), 0, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)) {}
  ~impl_t() {if (handle) CloseHandle(handle);}

  static void clear() {SysError::clear();}
  string error() {return SysError().toString();}
  void *get(const string &name) {return GetProcAddress(handle, name.c_str());}

#else // !_WIN32
  void *handle;

  impl_t(const string &path) : handle(dlopen(path.c_str(), RTLD_LAZY)) {}
  ~impl_t() {if (handle) dlclose(handle);}

  static void clear() {dlerror();} // Clear errors
  string error() {return dlerror();}
  void *get(const string &name) {return dlsym(handle, name.c_str());}
#endif // !_WIN32
};


DynamicLibrary::DynamicLibrary(const string &path) :
  path(path), impl(load(path)) {
  if (!impl->handle)
    THROW("Failed to open dynamic library '" << path << "': " << impl->error());
}


DynamicLibrary::DynamicLibrary(const vector<string> &paths) {
  for (auto &path: paths) {
    impl = load(this->path = path);
    if (impl->handle) return;
  }

  THROW("Failed to open any of the following dynamic libraries: "
    << String::join(paths, ", "));
}


DynamicLibrary::~DynamicLibrary() {}


void *DynamicLibrary::getSymbol(const string &name) {
  impl_t::clear();
  auto symbol = impl->get(name);

  if (!symbol)
    THROW("Failed to load dynamic symbol '" << name << "' from library '"
      << path << "': " << impl->error());

  return symbol;
}


SmartPointer<DynamicLibrary::impl_t> DynamicLibrary::load(const string &path) {
  if (!enabled) THROW("DynamicLibrary disabled globally");
  if (path.empty()) THROW("Library path is ''");

  impl_t::clear();
  return new impl_t(path);
}
