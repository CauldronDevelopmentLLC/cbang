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

#pragma once

#include "Subscriber.h"

#include <cbang/event/Event.h>
#include <cbang/db/EventLevelDB.h>

#include <map>


namespace cb {
  namespace API {
    class API;
    class TimeseriesHandler;

    class Timeseries : public RefCounted {
    public:
      using cb_t = Subscriber::cb_t;

    protected:
      TimeseriesHandler &handler;
      std::string        key;
      EventLevelDB       db;
      JSON::ValuePtr     last;
      std::map<uint64_t, SmartPointer<Subscriber>::Weak> subscribers;

    public:
      Timeseries(TimeseriesHandler &handler, const std::string &key);

      void query(uint64_t since, unsigned maxResults, const cb_t &cb);
      void broadcast(uint64_t time, const JSON::ValuePtr &value);

      SmartPointer<Subscriber> subscribe(
        uint64_t id, uint64_t since, unsigned maxResults, const cb_t &cb);
      void unsubscribe(uint64_t id);

    protected:
      void query(uint64_t ts);
      void query();
    };
  }
}
