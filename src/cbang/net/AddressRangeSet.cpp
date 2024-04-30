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

#include "AddressRangeSet.h"

#include <cbang/String.h>
#include <cbang/SStream.h>
#include <cbang/json/Sink.h>
#include <cbang/dns/Base.h>

using namespace std;
using namespace cb;


void AddressRangeSet::insert(const string &spec, DNS::Base *dns) {
  vector<string> tokens;
  String::tokenize(spec, tokens, " \r\n\t,;");

  for (auto &token: tokens)
    try {
      insert(AddressRange(token));

    } catch (const Exception &e) {
      if (!dns) throw;

      auto colon  = spec.find_last_of(':');
      string name = token.substr(0, colon);

      auto cb = [this, token, name] (
        DNS::Error error, const vector<SockAddr> &addrs) {
        for (auto &addr: addrs)
          insert(AddressRange(addr));

        requests.erase(name);
      };

      if (requests.find(name) == requests.end())
        requests[name] = dns->resolve(name, cb);
    }
}


void AddressRangeSet::insert(const AddressRange &range) {
  auto &s = range.getStart();
  auto &e = range.getEnd();
  unsigned sPos;
  unsigned ePos;
  auto sInside = find(s, &sPos);
  auto eInside = find(e, &ePos);

  // Check start adjacency
  if (!sInside && sPos && ranges[sPos - 1].getEnd().adjacent(s)) {
    sInside = true;
    sPos--;
    ranges[sPos].setEnd(s);
  }

  // Check end adjacency
  if (!eInside && ePos < ranges.size() && ranges[ePos].getStart().adjacent(e)) {
    eInside = true;
    ranges[ePos].setStart(e);
  }

  // Insert new range
  if (sPos == ePos && !sInside && !eInside) {
    ranges.insert(ranges.begin() + sPos, 1, AddressRange());
    if (sPos != ePos) ePos++;
  }

  // Set range start
  auto &r = ranges[sPos];
  if (r.getStart().isNull() || s < r.getStart()) r.setStart(s);

  // Set range end
  if (!eInside) r.setEnd(e);
  else if (sPos != ePos) r.setEnd(ranges[ePos].getEnd());

  // Remove overlapped ranges
  int rS = sPos + 1;
  int rE = ePos + (eInside ? 1 : 0);
  if (rS < rE) ranges.erase(ranges.begin() + rS, ranges.begin() + rE);
}


void AddressRangeSet::insert(const AddressRangeSet &o) {
  for (auto &range: o.ranges)
    insert(range);
}


string AddressRangeSet::toString() const {return SSTR(*this);}


void AddressRangeSet::print(ostream &stream) const {
  for (unsigned i = 0; i < ranges.size(); i++) {
    if (i) stream << ' ';
    stream << ranges[i];
  }
}


void AddressRangeSet::write(JSON::Sink &sink) const {
  sink.beginList();

  for (auto &range: ranges)
    sink.append(range.toString());

  sink.endList();
}


bool AddressRangeSet::find(const SockAddr &addr, unsigned *pos) const {
  unsigned sPos = 0;
  unsigned ePos = ranges.size();
  int cmp       = 1;

  while (sPos < ePos) {
    unsigned mid = (ePos - sPos) / 2 + sPos;
    cmp          = ranges[mid].cmp(addr);

    if (!cmp) {sPos = mid; break;}
    if (cmp < 0) ePos = mid;
    else sPos = mid + 1;
  }

  if (pos) *pos = sPos;

  return !cmp;
}
