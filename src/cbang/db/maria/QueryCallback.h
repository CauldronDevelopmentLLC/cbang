/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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

#include "EventDB.h"


namespace cb {
  namespace MariaDB {
    class QueryCallback {
      EventDB &db;
      EventDB::callback_t cb;
      std::string query;
      unsigned retry;

      typedef enum {
        STATE_START,
        STATE_QUERY,
        STATE_STORE,
        STATE_FETCH,
        STATE_FREE,
        STATE_NEXT,
        STATE_DONE,
      } state_t;

      state_t state = STATE_START;

    public:
      QueryCallback(EventDB &db, EventDB::callback_t cb,
                    const std::string &query, unsigned retry = 5);

      void call(EventDB::state_t state);
      bool next();
      void operator()(Event::Event &event, int fd, unsigned flags);
    };
  }
}
