/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#include "RefCounter.h"
#include "Errors.h"

#include <cbang/String.h>
#include <cbang/log/Logger.h>
#include <cbang/util/SmartToggle.h>

#include <inttypes.h>
#include <cxxabi.h>

using namespace cb;
using namespace std;


RefCounterPhonyImpl RefCounterPhonyImpl::singleton;


void RefCounter::log(const char *name, unsigned count) {
  if (!enableLogging) return;

  static bool entered = false;
  if (entered) return;
  SmartToggle toggle(entered);

  // Demangle C++ name
  int status = 0;
  char *demangled = abi::__cxa_demangle(name, 0, 0, &status);
  if (!status && demangled) name = demangled;

  string domain = string("RefCounter<") + name + ">";
  unsigned level = LOG_DEBUG_LEVEL(4);

  if (CBANG_LOG_ENABLED(domain, level))
    *CBANG_LOG_STREAM(domain, level)
      << name << String::printf(" 0x%" PRIxPTR " ", (uintptr_t)this)
      << (isProtected() ? "+ " : "- ") << count;

  if (demangled) free(demangled);
}


void RefCounter::raise(const string &msg) {REFERENCE_ERROR(msg);}
RefCounter *RefCounter::getRefPtr(const RefCounted *ref) {return ref->counter;}


void RefCounter::setRefPtr(const RefCounted *ref) {
  const_cast<RefCounted *>(ref)->counter = this;
}


void RefCounter::clearRefPtr(const RefCounted *ref) {
  const_cast<RefCounted *>(ref)->counter = 0;
}
