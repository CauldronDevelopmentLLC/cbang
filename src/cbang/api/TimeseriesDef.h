/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "QueryDef.h"
#include "Resolver.h"
#include "Websocket.h"

#include <cbang/db/EventLevelDB.h>

#include <map>


namespace cb {
  namespace API {
    class Timeseries;

    class TimeseriesDef : public RefCounted {
    public:
      API              &api;
      const std::string name;
      EventLevelDB      db;
      EventLevelDB      dbKeys;
      QueryDefPtr       queryDef;
      bool              automatic;
      uint64_t          period;
      uint64_t          timeout;

      std::map<std::string, SmartPointer<Timeseries>> series;

      TimeseriesDef(
        API &api, const std::string &name, const JSON::ValuePtr &config);
      TimeseriesDef(API &api, const JSON::ValuePtr &config);

      const std::string &getName() const {return name;}
      uint64_t getTimePeriod(uint64_t ts) const {return (ts / period) * period;}

      SmartPointer<Timeseries> get(const ResolverPtr &resolver, bool create);

      void add(const SmartPointer<Timeseries> &ts);
      void load();
    };

    using TimeseriesDefPtr = SmartPointer<TimeseriesDef>;
  }
}
