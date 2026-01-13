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

#include "Rate.h"

using namespace cb;


void Rate::reset() {
  total = last = head = 0;
  fill = 1;
  buckets[0] = 0;
}


double Rate::get(uint64_t now) const {
  if (!last) return 0; // No events
  unsigned delta = now / period - last;
  if (buckets.size() <= delta) return 0; // Too long since last event

  // Accounting for the delta ignore buckets which are too old
  unsigned maxFill = buckets.size() - delta;
  unsigned fill = maxFill < this->fill ? maxFill : this->fill;

  if (fill < 2) return 0; // Need at least two buckets

  // Count up the buckets
  double count = 0;
  for (unsigned i = 0; i < fill; i++)
    count += buckets[(head + buckets.size() - i) % buckets.size()];

  // Divide by the total time
  return count / ((fill + delta) * period);
}


void Rate::event(double value, uint64_t now) {
  unsigned time = now / period;

  if (last) {
    unsigned delta = time - last;

    // Advance, clearing any expired buckets along the way
    for (unsigned i = 0; i < delta && i < buckets.size(); i++) {
      if (fill < buckets.size()) fill++;
      if (++head == buckets.size()) head = 0;
      buckets[head] = 0;
    }
  }

  buckets[head] += value; // Sum event
  total += value;
  last = time;

  onUpdate();
}
