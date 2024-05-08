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

#pragma once

#include "SockAddr.h"

#include <string>
#include <ostream>


namespace cb {
  /**
   * Represents an IP address range.
   *
   * This class parses IP address range strings of the following format:
   *
   *   <address>[/BB]
   *     or
   *   <address>-<address>
   *
   * Where an <address> is either an IPv4 or IPv6 format address string.
   */
  class AddressRange {
    SockAddr start;
    SockAddr end;

  public:
    AddressRange() {}
    AddressRange(const std::string &spec) {set(spec);}
    AddressRange(const SockAddr &start, const SockAddr &end) {set(start, end);}
    AddressRange(const SockAddr &addr) {setBoth(addr);}

    const SockAddr &getStart() const {return start;}
    const SockAddr &getEnd()   const {return end;}

    void set(const std::string &spec);
    void set(const SockAddr &start, const SockAddr &end);
    void setStart(const SockAddr &start);
    void setEnd  (const SockAddr &end);
    void setBoth (const SockAddr &addr) {set(addr, addr);}

    std::string toString() const;
    int8_t getCIDRBits() const {return start.getCIDRBits(end);}

    int cmp(const SockAddr &addr) const;
    bool contains(const SockAddr &addr) const {return cmp(addr) == 0;}
    bool overlaps(const AddressRange &o) const;
    bool adjacent(const SockAddr &addr) const;
    bool adjacent(const AddressRange &o) const;
    void add(const AddressRange &o);
  };


  static inline
  std::ostream &operator<<(std::ostream &stream, const AddressRange &r) {
    return stream << r.toString();
  }
}
