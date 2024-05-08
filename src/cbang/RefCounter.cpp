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

#include "RefCounter.h"
#include "Errors.h"

#include <cbang/String.h>
#include <cbang/log/Logger.h>
#include <cbang/debug/Debugger.h>

#include <cstdarg>

using namespace cb;
using namespace std;


RefCounterPhonyImpl RefCounterPhonyImpl::singleton;


void RefCounter::raise(const string &msg) {REFERENCE_ERROR(msg);}
RefCounter *RefCounter::getRefPtr(const RefCounted *ref) {return ref->counter;}


void RefCounter::setRefPtr(const RefCounted *ref) {
  const_cast<RefCounted *>(ref)->counter = this;
}


void RefCounter::clearRefPtr(const RefCounted *ref) {
  const_cast<RefCounted *>(ref)->counter = 0;
}


void RefCounter::log(unsigned level, const char *fmt, ...) {
  if (!level) return;

  va_list ap;
  va_start(ap, fmt);
  LOG_DEBUG(level, this << ' ' << String::vprintf(fmt, ap) << '\n'
            << Debugger::getStackTrace());
  va_end(ap);
}
