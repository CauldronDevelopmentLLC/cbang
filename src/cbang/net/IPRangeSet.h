/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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
#include "IPAddressRange.h"

#include <cbang/SmartPointer.h>

#include <string>
#include <vector>
#include <iostream>

namespace cb {namespace JSON {class Sink;}}


namespace cb {
  /// Maintains a set of IP address ranges
  class IPRangeSet {
    typedef std::vector<uint32_t> rangeSet_t;
    rangeSet_t rangeSet;

  public:
    IPRangeSet() {}
    IPRangeSet(const std::string &spec) {insert(spec);}
    ~IPRangeSet() {clear();}

    void clear() {rangeSet.clear();}
    bool empty() {return rangeSet.empty();}
    unsigned size() {return rangeSet.size() / 2;}
    IPAddressRange get(unsigned i) const;

    void insert(const std::string &spec);
    void insert(const IPAddressRange &range);
    void insert(const IPRangeSet &set);
    void erase(const std::string &spec);
    void erase(const IPAddressRange &range);
    void erase(const IPRangeSet &set);
    bool contains(const SockAddr &addr) const;

    std::string toString() const;
    void print(std::ostream &stream) const;
    void write(JSON::Sink &sink) const;

    static SmartPointer<IPRangeSet> parse(const std::string &s)
    {return new IPRangeSet(s);}

  private:
    void insert(uint32_t start, uint32_t end);
    void erase(uint32_t start, uint32_t end);

    unsigned find(uint32_t ip) const;
    void shift(int amount, unsigned position);
  };


  static inline
  std::ostream &operator<<(std::ostream &stream, const IPRangeSet &s) {
    s.print(stream);
    return stream;
  }
}
