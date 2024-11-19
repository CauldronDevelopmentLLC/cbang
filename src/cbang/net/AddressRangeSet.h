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

#include "AddressRange.h"

#include <cbang/SmartPointer.h>

#include <string>
#include <map>
#include <vector>
#include <iostream>

namespace cb {
  namespace JSON {class Sink;}
  namespace DNS  {class Base; class Request;}
}


namespace cb {
  class AddressRangeSet : public RefCounted {
    typedef std::vector<AddressRange> ranges_t;
    ranges_t ranges;

  public:
    AddressRangeSet() {}
    AddressRangeSet(const std::string &spec) {insert(spec);}

    void clear() {ranges.clear();}
    bool empty() {return ranges.empty();}

    void insert(const std::string &spec, DNS::Base *dns = 0);
    void insert(const AddressRange &range);
    void insert(const AddressRangeSet &set);
    bool contains(const SockAddr &addr) const {return find(addr);}

    std::string toString() const;
    void print(std::ostream &stream) const;
    void write(JSON::Sink &sink) const;

    static SmartPointer<AddressRangeSet> parse(const std::string &s)
    {return new AddressRangeSet(s);}

  private:
    bool find(const SockAddr &addr, unsigned *pos = 0) const;
  };


  static inline
  std::ostream &operator<<(std::ostream &stream, const AddressRangeSet &s) {
    s.print(stream);
    return stream;
  }
}
