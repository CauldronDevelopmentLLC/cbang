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

#pragma once

#include "Exception.h"
#include "SStream.h"


// Exception class
#ifndef CBANG_EXCEPTION
#define CBANG_EXCEPTION cb::Exception
#endif

#define CBANG_EXCEPTION_SUBCLASS(name) (name)cb::Exception
#define CBANG_DEFINE_EXCEPTION_SUBCLASS(name)                       \
  struct name : public cb::Exception {                              \
    name(const cb::Exception &e) : cb::Exception(e) {}              \
  }


// Throws
#define CBANG_THROW(msg) throw CBANG_EXCEPTION((msg), CBANG_FILE_LOCATION)
#define CBANG_THROWC(msg, cause) \
  throw CBANG_EXCEPTION((msg), CBANG_FILE_LOCATION, (cause))
#define CBANG_THROWX(msg, code) \
  throw CBANG_EXCEPTION((msg), CBANG_FILE_LOCATION, code)
#define CBANG_THROWCX(msg, cause, code) \
  throw CBANG_EXCEPTION((msg), CBANG_FILE_LOCATION, (cause), code)

// Stream to string versions
#define CBANG_THROWS(msg) \
  throw CBANG_EXCEPTION(CBANG_SSTR(msg), CBANG_FILE_LOCATION)
#define CBANG_THROWCS(msg, cause) \
  throw CBANG_EXCEPTION(CBANG_SSTR(msg), CBANG_FILE_LOCATION, (cause))
#define CBANG_THROWXS(msg, code) \
  throw CBANG_EXCEPTION(CBANG_SSTR(msg), CBANG_FILE_LOCATION, code)
#define CBANG_THROWCXS(msg, cause, code) \
  throw CBANG_EXCEPTION(CBANG_SSTR(msg), CBANG_FILE_LOCATION, (cause), code)

// Asserts
#ifdef DEBUG
#define CBANG_ASSERT(cond, msg) do {if (!(cond)) CBANG_THROW(msg);} while (0)
#define CBANG_ASSERTS(cond, msg) do {if (!(cond)) CBANG_THROWS(msg);} while (0)
#define CBANG_ASSERTXS(cond, msg, code) \
  do {if (!(cond)) CBANG_THROWXS(msg, code);} while (0)

#else // DEBUG
#define CBANG_ASSERT(cond, msg)
#define CBANG_ASSERTS(cond, msg)
#define CBANG_ASSERTXS(cond, msg, code)
#endif // DEBUG


#ifdef USING_CBANG
#define EXCEPTION                       CBANG_EXCEPTION
#define EXCEPTION_SUBCLASS(name)        CBANG_EXCEPTION_SUBCLASS(name)
#define DEFINE_EXCEPTION_SUBCLASS(name) CBANG_DEFINE_EXCEPTION_SUBCLASS(name)

#define THROW(msg)                 CBANG_THROW(msg)
#define THROWC(msg, cause)         CBANG_THROWC(msg, cause)
#define THROWX(msg, code)          CBANG_THROWX(msg, code)
#define THROWCX(msg, cause, code)  CBANG_THROWCX(msg, cause, code)

#define THROWS(msg)                CBANG_THROWS(msg)
#define THROWCS(msg, cause)        CBANG_THROWCS(msg, cause)
#define THROWXS(msg, code)         CBANG_THROWXS(msg, code)
#define THROWCXS(msg, cause, code) CBANG_THROWCXS(msg, cause, code)

#define ASSERT(cond, msg)          CBANG_ASSERT(cond, msg)
#define ASSERTS(cond, msg)         CBANG_ASSERTS(cond, msg)
#define ASSERTXS(cond, msg, code)  CBANG_ASSERTXS(cond, msg, code)
#endif // USING_CBANG
