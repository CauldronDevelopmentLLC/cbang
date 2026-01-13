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

#include <cbang/json/Value.h>

#include <functional>
#include <vector>

namespace cb {
  namespace API {
    class Timeseries;

    class Subscriber {
    public:
      using cb_t = std::function<void (
        const SmartPointer<Exception> &err, const JSON::ValuePtr &data)>;

    protected:
      cb_t cb;
      SmartPointer<Timeseries> timeseries;
      uint64_t id;
      bool init = true;
      std::vector<JSON::ValuePtr> pending;

    public:
      Subscriber(cb_t cb, const SmartPointer<Timeseries> &timeseries,
        uint64_t id) : cb(cb), timeseries(timeseries), id(id) {}
      ~Subscriber();

      void error(const SmartPointer<Exception> &e);
      void first(const JSON::ValuePtr &results);
      void next(const JSON::ValuePtr &results);
    };
  }
}
