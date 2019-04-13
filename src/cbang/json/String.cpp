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

#include "String.h"

#include <limits>

#include <stdlib.h>
#include <errno.h>

using namespace std;
using namespace cb::JSON;


double String::getNumber() const {
  string l = cb::String::toLower(s);

  if (l == "nan") return std::numeric_limits<double>::quiet_NaN();

  if (l == "-infinity" || l == "-inf")
    return -numeric_limits<double>::infinity();

  if (l == "infinity" || l == "inf")
    return numeric_limits<double>::infinity();

  errno = 0;
  char *end = 0;
  double v = strtod(s.c_str(), &end);
  if (errno || (end && *end)) TYPE_ERROR("Not a number");

  return v;
}


int32_t String::getS32() const {
  errno = 0;
  char *end = 0;
  long v = strtol(s.c_str(), &end, 0);

  if (errno || v < -numeric_limits<int32_t>::max() ||
      numeric_limits<int32_t>::max() < v || (end && *end))
    TYPE_ERROR("Not a signed 32-bit number");

  return (int32_t)v;
}


uint32_t String::getU32() const {
  errno = 0;
  char *end = 0;
  unsigned long v = strtoul(s.c_str(), &end, 0);

  if (errno || v < -numeric_limits<uint32_t>::max() ||
      numeric_limits<uint32_t>::max() < v || (end && *end))
    TYPE_ERROR("Not an unsigned 32-bit number");

  return (uint32_t)v;
}


int64_t String::getS64() const {
  errno = 0;
  char *end = 0;
  long long v = strtoll(s.c_str(), &end, 0);

  if (errno || v < -numeric_limits<int64_t>::max() ||
      numeric_limits<int64_t>::max() < v || (end && *end))
    TYPE_ERROR("Not a signed 64-bit number");

  return (int64_t)v;
}


uint64_t String::getU64() const {
  errno = 0;
  char *end = 0;
  unsigned long long v = strtoull(s.c_str(), &end, 0);

  if (errno || v < -numeric_limits<uint64_t>::max() ||
      numeric_limits<uint64_t>::max() < v || (end && *end))
    TYPE_ERROR("Not an unsigned 64-bit number");

  return (uint64_t)v;
}
