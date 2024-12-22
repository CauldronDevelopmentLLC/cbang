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
#include <cbang/json/Serializable.h>

#include <map>
#include <vector>
#include <limits>


namespace cb {
  namespace Event {
    class RateTracker : public JSON::Serializable {
      SmartPointer<RateSet> rates;
      const unsigned size;   //< Maximum number of measurements
      const unsigned period; //< Measurement period in seconds

      unsigned offset = 0;   //< Index to most recent measurement
      unsigned count  = 0;   //< Number of measurements
      std::vector<uint64_t> times;

      static constexpr const double NaN =
        std::numeric_limits<double>::quiet_NaN();

      struct Entry {
        double rate  = NaN;
        double total = NaN;
      };

      typedef std::vector<Entry> series_t;
      typedef std::map<std::string, series_t> measurements_t;
      measurements_t measurements;

      EventPtr event;

    public:
      RateTracker(Base &base, const SmartPointer<RateSet> &rates,
        unsigned size = 60 * 24, unsigned period = 60);

      const SmartPointer<RateSet> &getRateSet() const {return rates;}

      void clear();

      void insert(JSON::Sink &sink) const;

      // From JSON::Serializable
      using JSON::Serializable::write;
      void write(JSON::Sink &sink) const override;

    protected:
      void measure();
    };
  }
}
