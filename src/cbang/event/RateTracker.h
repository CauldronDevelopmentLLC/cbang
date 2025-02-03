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

#include "Base.h"
#include "Event.h"

#include <cbang/util/RateSet.h>
#include <cbang/util/CallbackSet.h>
#include <cbang/json/Sink.h>

#include <vector>


namespace cb {
  namespace Event {
    class RateTracker {
      SmartPointer<RateSet> rates;
      const unsigned size;   //< Maximum number of measurements
      const unsigned period; //< Measurement period in seconds

      using cb_set_t =
        CallbackSet<uint64_t, const std::string &, double, double>;
      using cb_t = cb_set_t::callback_t;
      cb_set_t callbacks;

      struct Series {
        std::vector<double> rates;
        std::vector<double> totals;

        Series(unsigned size);
      };

      std::map<std::string, SmartPointer<Series>> measurements;
      std::vector<uint64_t> times;

      unsigned offset = 0; //< Index to most recent measurement
      unsigned fill   = 0; //< Number of measurements taken

      EventPtr event;

    public:
      RateTracker(Base &base, const SmartPointer<RateSet> &rates,
        unsigned size = 60 * 24, unsigned period = 60);

      uint64_t addCallback(const cb_t &cb) {return callbacks.add(cb);}
      void removeCallback(uint64_t id) {callbacks.remove(id);}

      const SmartPointer<RateSet> &getRateSet() const {return rates;}

      void clear();

      void write(JSON::Sink &sink) const;

    protected:
      void measure();
    };
  }
}
