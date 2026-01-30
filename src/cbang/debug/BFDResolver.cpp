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

#include "BFDResolver.h"

#include <cbang/config.h>

#ifdef HAVE_BFD
#include <cbang/String.h>
#include <cbang/log/Logger.h>

#include <cstdarg>
#include <cstring>

#ifdef HAVE_VALGRIND
#include <valgrind/memcheck.h>
#endif

#include <bfd.h>
#include <dlfcn.h>


#ifndef bfd_get_section_flags
#define bfd_get_section_flags(ABFD, SEC) bfd_section_flags(SEC)
#endif

#ifndef bfd_get_section_vma
#define bfd_get_section_vma(ABFD, SEC) bfd_section_vma(SEC)
#endif

#ifndef bfd_get_section_size
#define bfd_get_section_size(SEC) bfd_section_size(SEC)
#endif

using namespace std;
using namespace cb;


namespace {
#ifdef HAVE_BFD_ERROR_HANDLER_VPRINTFLIKE
  void _bfd_error_handler(const char *fmt, va_list ap) {
    // Suppress annoying error message caused by DWARF/libbfd incompatibility
    if (String::startsWith(fmt, "DWARF error: could not find variable spec"))
      return;

    LOG_ERROR(String::vprintf(fmt, ap));
  }

#else // HAVE_BFD_ERROR_HANDLER_VPRINTFLIKE
  void _bfd_error_handler(const char *fmt, ...) {
    // Suppress annoying error message caused by DWARF/libbfd incompatibility
    if (String::startsWith(fmt, "DWARF error: could not find variable spec"))
      return;

    va_list ap;

    va_start(ap, fmt);
    LOG_ERROR(String::vprintf(fmt, ap));
    va_end(ap);
  }
#endif // _bfd_error_handler
}


struct BFDResolver::private_t {
  bfd *abfd;
  asymbol **syms;
};


BFDResolver::BFDResolver(const string &path) : p(new private_t), path(path) {
  memset(p, 0, sizeof(private_t));

  bfd_init();
  bfd_set_error_handler(_bfd_error_handler);

  if (!(p->abfd = bfd_openr(path.c_str(), 0)))
    throw runtime_error(string("Error opening BDF in '") + path + "'");

  if (bfd_check_format(p->abfd, bfd_archive))
    throw runtime_error("Not a BFD archive");

  char **matching;
  if (!bfd_check_format_matches(p->abfd, bfd_object, &matching)) {
    if (bfd_get_error() == bfd_error_file_ambiguously_recognized)
      free(matching);
    throw runtime_error("Does not match BFD object");
  }

  if ((bfd_get_file_flags(p->abfd) & HAS_SYMS) == 0)
    throw runtime_error("No symbols");

  long storage = bfd_get_symtab_upper_bound(p->abfd);
  if (storage <= 0) THROW("Invalid storage size");

  p->syms = (asymbol **)new char[storage];

  long count = bfd_canonicalize_symtab(p->abfd, p->syms);
  if (count < 0) throw runtime_error("Invalid symbol count");
}


BFDResolver::~BFDResolver() {
  if (p->abfd) bfd_close(p->abfd);
  if (p->syms) delete [] p->syms;
}


void BFDResolver::resolve(
    void *addr, string &filename, string &function, int &line) {
  bfd_vma pc = (bfd_vma)addr;

  // Adjust for base which is non-zero for PIC/PIE code
  Dl_info info;
  if (dladdr(addr, &info)) pc -= (uintptr_t)info.dli_fbase;

  // Find section
  asection *section = 0;
  for (asection *s = p->abfd->sections; s; s = s->next) {
    if ((bfd_get_section_flags(p->abfd, s) & SEC_CODE) == 0)
      continue;

    bfd_vma vma = bfd_get_section_vma(p->abfd, s);
    if (pc < vma) {
      section = s->prev;
      break;
    }
  }

  if (section && p->syms) {
    const char *_filename = 0;
    const char *_function = 0;
    unsigned _line = 0;

    bfd_vma vma = bfd_get_section_vma(p->abfd, section);
    if (bfd_find_nearest_line(p->abfd, section, p->syms, pc - vma,
                              &_filename, &_function, &_line)) {

#ifdef VALGRIND_MAKE_MEM_DEFINED
      if (_filename)
        (void)VALGRIND_MAKE_MEM_DEFINED(_filename, strlen(_filename));
      if (_function)
        (void)VALGRIND_MAKE_MEM_DEFINED(_function, strlen(_function));
      (void)VALGRIND_MAKE_MEM_DEFINED(&line, sizeof(_line));
#endif // VALGRIND_MAKE_MEM_DEFINED

      if (_filename) {filename = _filename; line = _line;}
      if (_function) function = _function;
    }
  }
}

#else  // HAVE_BFD
cb::BFDResolver::BFDResolver(const std::string &) {}
cb::BFDResolver::~BFDResolver() {}
void cb::BFDResolver::resolve(void *, std::string &, std::string &, int &) {}
#endif // HAVE_BFD
