/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2020, Cauldron Development LLC
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

#pragma once

#include <cbang/StdTypes.h>


namespace cb {
  template <int BUCKETS = 60 * 5, int PERIOD = 1, typename T = uint64_t>
  class Rate {
  public:
    int last;
    int head;
    int fill;
    T buf[BUCKETS];

  public:
    Rate() : last(0), head(0), fill(1) {buf[0] = 0;}


    double get(uint64_t now = Time::now()) const {
      if (!last) return 0; // No events
      int delta = now / PERIOD - last;
      if (BUCKETS <= delta) return 0; // Too long since last event

      // Accounting for the delta ignore buckets which are too old
      int maxFill = BUCKETS - delta;
      int fill = maxFill < this->fill ? maxFill : this->fill;

      if (fill < 2) return 0; // Need at least two buckets

      // Count up the buckets
      double count = 0;
      for (int i = 0; i < fill; i++)
        count += buf[(head + BUCKETS - i) % BUCKETS];

      // Divide by the total time
      return count / ((fill + delta) * PERIOD);
    }


    void event(T value = 1, uint64_t now = Time::now()) {
      int bucket = now / PERIOD;

      if (last) {
        int delta = bucket - last;

        // Advance, clearing any expired buckets along the way
        for (int i = 0; i < delta && i < BUCKETS; i++) {
          if (fill < BUCKETS) fill++;
          if (++head == BUCKETS) head = 0;
          buf[head] = 0;
        }
      }

      buf[head] += value; // Sum event
      last = bucket;
    }
  };
}
