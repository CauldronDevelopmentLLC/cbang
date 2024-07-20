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

/*
 * Hear you will find several preprocessor macros that can be used as a
 * convenient way to add the current file, line and column where an exception
 * occurred.  These are:
 *
 *   THROW(<message stream>)
 *   THROWC(<message stream>, Exception &cause)
 *   THROWX(<message stream>, int code)
 *   THROWCX(<message stream>, Exception &cause, int code)
 *
 *   ASSERT(bool condition, <message stream>)
 *
 * <message stream> can be a chain of items to be piped to an std::ostream
 * in order to format the message.  For example:
 *
 *   THROW("The error number was: " << errno);
 *
 * These stream chains can be of arbitrary length.
 *
 * The ASSERT macros evaluate condition and throw an exception if false.
 * ASSERTs are compiled out if DEBUG is not defined.
 */

#pragma once

#include "Exception.h"
#include "SStream.h"


// Exception class
#ifndef CBANG_EXCEPTION
#define CBANG_EXCEPTION cb::Exception
#endif


#define CBANG_DEFINE_EXCEPTION_SUPER(NAME, SUPER)               \
  struct NAME : public SUPER {using cb::Exception::Exception;}

#define CBANG_DEFINE_EXCEPTION_SUBCLASS(NAME)           \
  CBANG_DEFINE_EXCEPTION_SUPER(NAME, cb::Exception)


// Exception type throws
#define CBANG_THROWT(TYPE, MSG)                         \
  throw TYPE(CBANG_SSTR(MSG), CBANG_FILE_LOCATION)
#define CBANG_THROWTC(TYPE, MSG, CAUSE)                         \
  throw TYPE(CBANG_SSTR(MSG), CBANG_FILE_LOCATION, CAUSE)
#define CBANG_THROWTX(TYPE, MSG, CODE)                          \
  throw TYPE(CBANG_SSTR(MSG), CBANG_FILE_LOCATION, CODE)
#define CBANG_THROWTCX(TYPE, MSG, CAUSE, CODE)                  \
  throw TYPE(CBANG_SSTR(MSG), CBANG_FILE_LOCATION, CAUSE, CODE)

// Throws
#define CBANG_THROW(MSG) CBANG_THROWT(CBANG_EXCEPTION, MSG)
#define CBANG_THROWC(MSG, CAUSE) CBANG_THROWTC(CBANG_EXCEPTION, MSG, CAUSE)
#define CBANG_THROWX(MSG, CODE) CBANG_THROWTX(CBANG_EXCEPTION, MSG, CODE)
#define CBANG_THROWCX(MSG, CAUSE, CODE)                 \
  CBANG_THROWTCX(CBANG_EXCEPTION, MSG, CAUSE, CODE)

// Deprecated
#define CBANG_THROWS(MSG) CBANG_THROW(MSG)
#define CBANG_THROWCS(MSG, CAUSE) CBANG_THROWC(MSG, CAUSE)
#define CBANG_THROWXS(MSG, CODE) CBANG_THROWX(MSG, CODE)
#define CBANG_THROWCXS(MSG, CAUSE, CODE) CBANG_THROWCX(MSG, CAUSE, CODE)

// Asserts
#ifdef DEBUG
#define CBANG_ASSERTX(COND, MSG, CODE)                  \
  do {if (!(COND)) CBANG_THROWX(MSG, CODE);} while (0)
#define CBANG_ASSERT(COND, MSG) CBANG_ASSERTX(COND, MSG, 0)

// Deprecated
#define CBANG_ASSERTS(COND, MSG) CBANG_ASSERT(COND, MSG)
#define CBANG_ASSERTXS(COND, MSG, CODE) CBANG_ASSERTX(COND, MSG, CODE)

#else // DEBUG
#define CBANG_ASSERT(COND, MSG)
#define CBANG_ASSERTS(COND, MSG)
#define CBANG_ASSERTXS(COND, MSG, CODE)
#endif // DEBUG


#ifdef USING_CBANG
#define EXCEPTION                       CBANG_EXCEPTION
#define EXCEPTION_SUBCLASS(NAME)        CBANG_EXCEPTION_SUBCLASS(NAME)
#define DEFINE_EXCEPTION_SUBCLASS(NAME) CBANG_DEFINE_EXCEPTION_SUBCLASS(NAME)

#define THROW(MSG)                 CBANG_THROW(MSG)
#define THROWC(MSG, CAUSE)         CBANG_THROWC(MSG, CAUSE)
#define THROWX(MSG, CODE)          CBANG_THROWX(MSG, CODE)
#define THROWCX(MSG, CAUSE, CODE)  CBANG_THROWCX(MSG, CAUSE, CODE)

#define THROWS(MSG)                CBANG_THROWS(MSG)
#define THROWCS(MSG, CAUSE)        CBANG_THROWCS(MSG, CAUSE)
#define THROWXS(MSG, CODE)         CBANG_THROWXS(MSG, CODE)
#define THROWCXS(MSG, CAUSE, CODE) CBANG_THROWCXS(MSG, CAUSE, CODE)

#define ASSERT(COND, MSG)          CBANG_ASSERT(COND, MSG)
#define ASSERTS(COND, MSG)         CBANG_ASSERTS(COND, MSG)
#define ASSERTXS(COND, MSG, CODE)  CBANG_ASSERTXS(COND, MSG, CODE)
#endif // USING_CBANG
