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

#include "RateSet.h"
#include "RateCollectionNS.h"

using namespace std;
using namespace cb;


SmartPointer<RateCollection> RateSet::getNS(const string &ns) {
  return new RateCollectionNS(SmartPtr(this), ns);
}


Rate &RateSet::getRate(const std::string &key) {
  return rates.insert(rates_t::value_type(key, Rate(size, period)))
    .first->second;
}


const Rate &RateSet::getRate(const std::string &key) const {
  auto it = rates.find(key);
  if (it == rates.end()) CBANG_THROW("Rate '" << key << "' not in set");
  return it->second;
}


void RateSet::insert(JSON::Sink &sink, bool withTotals) const {
  for (auto &p: *this)
    if (!withTotals) sink.insert(p.first, p.second.get());
    else {
      sink.insertDict(p.first);
      sink.insert("rate", p.second.get());
      sink.insert("total", p.second.getTotal());
      sink.endDict();
    }
}


void RateSet::write(JSON::Sink &sink, bool withTotals) const {
  sink.beginDict();
  insert(sink, withTotals);
  sink.endDict();
}
