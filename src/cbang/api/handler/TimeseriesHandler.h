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

#include <cbang/api/Handler.h>
#include <cbang/api/QueryDef.h>
#include <cbang/db/EventLevelDB.h>


namespace cb {
  namespace API {
    class API;
    class TimeseriesDef;

    class TimeseriesHandler : public Handler, public QueryDef  {
    public:
      std::string     name;
      EventLevelDB    db;
      uint64_t        period;
      JSON::ValuePtr  key;
      Event::EventPtr event;

      std::map<std::string, SmartPointer<Timeseries>> series;

      JSON::ValuePtr        last;
      JSON::ValuePtr        results;
      JSON::Value::iterator it;
      uint64_t              resultsTime = 0;
      std::string           resultsTimeKey;
      SmartPointer<LevelDB::Batch> batch;

      TimeseriesHandler(
        API &api, const std::string &name, const JSON::ValuePtr &config);

      const std::string &getName() const {return name;}

    protected:
      std::string resolveKey(const JSON::Value &dict) const;
      uint64_t getTimePeriod(uint64_t ts) const {return (ts / period) * period;}
      double getNext() const;
      SmartPointer<Timeseries> get(const std::string &key, bool create = true);

      void schedule();
      void process();

      using QueryDef::query;
      void query(uint64_t time);
      void action(const CtxPtr &ctx);

      // From QueryDef
      SmartPointer<MariaDB::EventDB> getDBConnection() const override;

      // From Handler
      bool operator()(const CtxPtr &ctx) override;
   };
  }
}
