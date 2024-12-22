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

#include "RateTracker.h"

#include <cbang/time/Time.h>

using namespace std;
using namespace cb;
using namespace cb::Event;


#define RBUF_INDEX(i) ((size + offset - (i)) % size)


RateTracker::RateTracker(Base &base, const SmartPointer<RateSet> &rates,
  unsigned size, unsigned period) :
  rates(rates), size(size), period(period), times(size),
  event(base.newEvent(this, &RateTracker::measure)) {event->add(period);}


void RateTracker::clear() {
  offset = count = 0;
  times.clear();
  measurements.clear();
}


void RateTracker::insert(JSON::Sink &sink) const {
  sink.insert("period", period);
  sink.insert("size",   size);

  sink.insertList("times");
  for (unsigned i = 0; i < count; i++)
    sink.append(Time(times[RBUF_INDEX(i)]).toString());
  sink.endList();

  sink.insertDict("measurements");
  for (auto &p1: measurements) {
    sink.insertDict(p1.first);
    auto &series = p1.second;

    sink.insertList("rates");
    for (unsigned i = 0; i < count; i++) {
      auto &value = series[RBUF_INDEX(i)].rate;
      if (value == NaN) sink.appendNull();
      else sink.append(value);
    }
    sink.endList();

    sink.insertList("totals");
    for (unsigned i = 0; i < count; i++) {
      auto &value = series[RBUF_INDEX(i)].total;
      if (value == NaN) sink.appendNull();
      else sink.append(value);
    }
    sink.endList();

    sink.endDict(); // series
  }
  sink.endDict(); // measurements
}


void RateTracker::write(JSON::Sink &sink) const {
  sink.beginDict();
  insert(sink);
  sink.endDict();
}


void RateTracker::measure() {
  if (size == ++offset) offset = 0;
  if (count < size) count++;

  uint64_t now = times[offset] = Time::now();

  for (auto &p: *rates) {
    auto &name = p.first;
    auto &rate = p.second;

    // Find series, insert if non-existant
    auto it = measurements.find(name);
    if (it == measurements.end())
      it = measurements.insert(
        it, measurements_t::value_type(name, series_t(size)));

    auto &entry = it->second;
    entry[offset].rate  = rate.get(now);
    entry[offset].total = rate.getTotal();
  }
}