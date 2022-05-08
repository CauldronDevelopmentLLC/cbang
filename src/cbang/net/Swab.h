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

#include <cbang/StdTypes.h>

#ifdef _WIN32
#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN    4321
#define BYTE_ORDER    LITTLE_ENDIAN

#elif __APPLE__
#include <machine/endian.h>

#elif __FreeBSD__
#include <sys/endian.h>

#else // Other POSIX systems
#include <endian.h>
#endif

#if 0
#define CBANG_SWAP16(OUT, IN)                                           \
  inline OUT swap16(IN x) {                                             \
    return (((uint16_t)x & (uint16_t)0xff) << 8) |                      \
      (((uint16_t)x & (uint16_t)0xff00U) >> 8);                         \
  }

#define CBANG_SWAP32(OUT, IN)                                           \
  inline OUT swap32(IN x) {                                             \
    return (swap16((uint32_t)x & (uint32_t)0xffffUL) << 16) |           \
      (swap16((uint32_t)x & (uint32_t)0xffff0000UL) >> 16);             \
  }

#define CBANG_SWAP64(OUT, IN)                                           \
  inline OUT swap64(IN x) {                                             \
    return (swap32((uint64_t)x & (uint64_t)0xffffffffULL) << 32) |      \
      (swap32((uint64_t)x & (uint64_t)0xffffffff00000000ULL) >> 32);    \
  }

CBANG_SWAP16(uint16_t, uint8_t);
CBANG_SWAP16(int16_t,  int8_t);
CBANG_SWAP16(uint16_t, uint16_t);
CBANG_SWAP16(int16_t,  int16_t);
CBANG_SWAP16(uint16_t, uint32_t);
CBANG_SWAP16(int16_t,  int32_t);
CBANG_SWAP16(uint16_t, uint64_t);
CBANG_SWAP16(int16_t,  int64_t);

CBANG_SWAP32(uint32_t, uint8_t);
CBANG_SWAP32(int32_t,  int8_t);
CBANG_SWAP32(uint32_t, uint16_t);
CBANG_SWAP32(int32_t,  int16_t);
CBANG_SWAP32(uint32_t, uint32_t);
CBANG_SWAP32(int32_t,  int32_t);
CBANG_SWAP32(uint32_t, uint64_t);
CBANG_SWAP32(int32_t,  int64_t);

CBANG_SWAP64(uint64_t, uint8_t);
CBANG_SWAP64(int64_t,  int8_t);
CBANG_SWAP64(uint64_t, uint16_t);
CBANG_SWAP64(int64_t,  int16_t);
CBANG_SWAP64(uint64_t, uint32_t);
CBANG_SWAP64(int64_t,  int32_t);
CBANG_SWAP64(uint64_t, uint64_t);
CBANG_SWAP64(int64_t,  int64_t);

#undef CBANG_SWAP16
#undef CBANG_SWAP32
#undef CBANG_SWAP64
#endif

#include <limits>

template<typename T>
uint16_t swap16(T _x) {
  uint16_t x;

  if (std::numeric_limits<T>::is_signed) x = (int16_t)_x;
  else x = _x;

  return (x << 8) | (x >> 8);
}


template<typename T>
uint32_t swap32(T _x) {
  uint32_t x;

  if (std::numeric_limits<T>::is_signed) x = (int32_t)_x;
  else x = _x;

  return ((uint32_t)swap16(x) << 16) | swap16(x >> 16);
}


template<typename T>
uint64_t swap64(T _x) {
  uint64_t x;

  if (std::numeric_limits<T>::is_signed) x = (int64_t)_x;
  else x = _x;

  return ((uint64_t)swap32(x) << 32) | swap32(x >> 32);
}


// TODO Should also swap the 64-bit parts.  Bug inherited from COSM.
inline uint128_t swap128(uint128_t x) {return uint128_t(x.lo, x.hi);}


#if BYTE_ORDER == LITTLE_ENDIAN
#define hton16(x)  swap16(x)
#define hton32(x)  swap32(x)
#define hton64(x)  swap64(x)
#define hton128(x) swap128(x)

#define htol16(x)  (x)
#define htol32(x)  (x)
#define htol64(x)  (x)
#define htol128(x) (x)

#else // BIG_ENDIAN
#define hton16(x)  (x)
#define hton32(x)  (x)
#define hton64(x)  (x)
#define hton128(x) (x)

#define htol16(x)  swap16(x)
#define htol32(x)  swap32(x)
#define htol64(x)  swap64(x)
#define htol128(x) swap128(x)
#endif
