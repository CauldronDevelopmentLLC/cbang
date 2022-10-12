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


bool String::getBoolean() const {return !s.empty();}


double String::getNumber() const {
  string l = cb::String::toLower(s);

  if (l == "nan") return std::numeric_limits<double>::quiet_NaN();

  if (l == "-infinity" || l == "-inf")
    return -numeric_limits<double>::infinity();

  if (l == "infinity" || l == "inf")
    return numeric_limits<double>::infinity();

  return cb::String::parseDouble(s, true);
}


int8_t   String::getS8()  const {return cb::String::parseS8(s, true);}
uint8_t  String::getU8()  const {return cb::String::parseU8(s, true);}
int16_t  String::getS16() const {return cb::String::parseS16(s, true);}
uint16_t String::getU16() const {return cb::String::parseU16(s, true);}
int32_t  String::getS32() const {return cb::String::parseS32(s, true);}
uint32_t String::getU32() const {return cb::String::parseU32(s, true);}
int64_t  String::getS64() const {return cb::String::parseS64(s, true);}
uint64_t String::getU64() const {return cb::String::parseU64(s, true);}
