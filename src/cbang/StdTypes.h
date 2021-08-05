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

#if defined(_WIN32) && !defined(__MINGW32__) && _MSC_VER < 1600
#include <limits.h>

namespace {
#if !defined(int8_t)
  typedef __int8            int8_t;
#endif

#if !defined(int16_t)
  typedef __int16           int16_t;
#endif

#if !defined(int32_t)
  typedef __int32           int32_t;
#endif

#if !defined(int64_t)
  typedef __int64           int64_t;
#endif

#if !defined(uint8_t)
  typedef unsigned __int8   uint8_t;
#endif

#if !defined(uint26_t)
  typedef unsigned __int16  uint16_t;
#endif

#if !defined(uint32_t)
  typedef unsigned __int32  uint32_t;
#endif

#if !defined(uint64_t)
  typedef unsigned __int64  uint64_t;
#endif
}

#else
#include <stdint.h>
#endif

// Format strings
#include <inttypes.h>

#ifndef __WORDSIZE
#  ifdef _WIN64
#    define __WORDSIZE 64
#  else
#    define __WORDSIZE 32
#  endif
#endif // __WORDSIZE

#ifndef __PRI64_PREFIX
#  if __WORDSIZE == 64
#    define __PRI64_PREFIX        "l"
#  else
#    define __PRI64_PREFIX        "ll"
#  endif
#endif // __PRI64_PREFIX

#ifndef __PRIPTR_PREFIX
#  if __WORDSIZE == 64
#    define __PRIPTR_PREFIX       "l"
#  else
#    define __PRIPTR_PREFIX
#  endif
#endif // __PRIPTR_PREFIX


#ifndef PRIo32
#  define PRIo32 "o"
#endif

#ifndef PRIu32
#  define PRIu32 "u"
#endif

#ifndef PRIi32
#  define PRIi32 "i"
#endif

#ifndef PRId32
#  define PRId32 "d"
#endif

#ifndef PRIo64
#  define PRIo64 __PRI64_PREFIX "o"
#endif

#ifndef PRIu64
#  define PRIu64 __PRI64_PREFIX "u"
#endif

#ifndef PRIi64
#  define PRIi64 __PRI64_PREFIX "i"
#endif

#ifndef PRId64
#  define PRId64 __PRI64_PREFIX "d"
#endif

#ifndef PRIxPTR
#  define PRIxPTR __PRIPTR_PREFIX "x"
#endif


struct uint128_t {
  uint64_t lo;
  uint64_t hi;

#ifdef __cplusplus
  uint128_t() : lo(0), hi(0) {}
  uint128_t(const uint64_t &lo) : lo(lo), hi(0) {}
  uint128_t(const uint64_t &hi, const uint64_t &lo) : lo(lo), hi(hi) {}
  operator bool () {return hi || lo;}
#endif // __cplusplus
};

#ifdef __cplusplus
#include <ostream>
#include <iomanip>

inline std::ostream &operator<<(std::ostream &stream, const uint128_t &x) {
  char fill = stream.fill();
  std::ios::fmtflags flags = stream.flags();

  stream.fill('0');
  stream.flags(std::ios::right | std::ios::hex);

  stream << "0x" << std::setw(16) << x.hi << std::setw(16) << x.lo;

  stream.fill(fill);
  stream.flags(flags);
  return stream;
}

inline bool operator<(const uint128_t &i1, const uint128_t &i2) {
  return i1.hi < i2.hi || (i1.hi == i2.hi && i1.lo < i2.lo);
}

inline bool operator>(const uint128_t &i1, const uint128_t &i2) {
  return i1.hi > i2.hi || (i1.hi == i2.hi && i1.lo > i2.lo);
}

inline bool operator<=(const uint128_t &i1, const uint128_t &i2) {
  return !(i1 > i2);
}

inline bool operator>=(const uint128_t &i1, const uint128_t &i2) {
  return !(i1 < i2);
}

inline bool operator==(const uint128_t &i1, const uint128_t &i2) {
  return i1.hi == i2.hi && i1.lo == i2.lo;
}

inline bool operator!=(const uint128_t &i1, const uint128_t &i2) {
  return !(i1 == i2);
}

#endif // __cplusplus
