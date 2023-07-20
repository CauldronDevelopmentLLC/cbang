/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include <ostream>
#include <cstdint>


class uint128_t {
public:
  uint64_t lo;
  uint64_t hi;

  uint128_t() : lo(0), hi(0) {}
  uint128_t(const uint64_t &lo) : lo(lo), hi(0) {}
  uint128_t(const uint64_t &hi, const uint64_t &lo) : lo(lo), hi(hi) {}

  void write(std::ostream &stream) const;

  operator bool () {return hi || lo;}

  bool operator<(const uint128_t &o) const {
    return hi < o.hi || (hi == o.hi && lo < o.lo);
  }

  bool operator>(const uint128_t &o) const {
    return hi > o.hi || (hi == o.hi && lo > o.lo);
  }

  bool operator<=(const uint128_t &o) const {return !(o < *this);}
  bool operator>=(const uint128_t &o) const {return !(*this < o);}
  bool operator==(const uint128_t &o) const {return hi == o.hi && lo == o.lo;}
  bool operator!=(const uint128_t &o) const {return !(*this == o);}
};


inline std::ostream &operator<<(std::ostream &stream, const uint128_t &x) {
  x.write(stream);
  return stream;
}
