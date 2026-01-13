/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
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

#include "RateTracker.h"

#include <cbang/time/Time.h>

#include <limits>

using namespace std;
using namespace cb;
using namespace cb::Event;


#define RBUF_INDEX(i) ((size + offset - (i)) % size)


RateTracker::Series::Series(unsigned size) :
  rates (size, numeric_limits<double>::quiet_NaN()),
  totals(size, numeric_limits<double>::quiet_NaN()) {}


RateTracker::RateTracker(Base &base, const SmartPointer<RateSet> &rates,
  unsigned size, unsigned period) :
  rates(rates), size(size), times(size),
  event(base.newEvent(this, &RateTracker::measure)) {event->add(period);}


void RateTracker::clear() {
  offset = 0;
  measurements.clear();
  times.clear();
}


void RateTracker::measure() {
  if (++offset == size) offset = 0;
  if (fill != size) fill++;

  uint64_t now = times[offset] = Time::now();

  for (auto &p: *rates) {
    auto &key  = p.first;
    auto rate  = p.second.get(now);
    auto total = p.second.getTotal();

    // Find series, insert if non-existant
    SmartPointer<Series> series;
    auto it = measurements.find(key);
    if (it != measurements.end()) series = it->second;
    else series = measurements[key] = new Series(size);

    series->rates [offset] = rate;
    series->totals[offset] = total;

    callbacks(now, key, rate, total);
  }
}


#define ARRAY_INDEX(i) ((offset + 1 + (i) + size - fill) % size)

void RateTracker::write(JSON::Sink &sink) const {
  sink.beginDict();

  sink.insertList("times");
  for (unsigned i = 0; i < fill; i++)
    sink.append(times[ARRAY_INDEX(i)]);
  sink.endList();

  sink.insertDict("measurements");
  for (auto &p: measurements) {
    sink.insertDict(p.first); // series

    auto &rates = p.second->rates;
    sink.insertList("rates");
    for (unsigned i = 0; i < fill; i++)
      sink.append(rates[ARRAY_INDEX(i)]);
    sink.endList(); // rates

    auto &totals = p.second->totals;
    sink.insertList("totals");
    for (unsigned i = 0; i < fill; i++)
      sink.append(totals[ARRAY_INDEX(i)]);
    sink.endList(); // totals

    sink.endDict(); // series
  }
  sink.endDict(); // measurements

  sink.endDict();
}
