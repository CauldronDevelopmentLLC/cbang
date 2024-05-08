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

#include "Column.h"
#include "Parameter.h"

#include <string>

struct sqlite3_stmt;
struct sqlite3;

namespace cb {
  namespace JSON {class Sink;}

  namespace DB {
    class Database;

    class Statement {
      sqlite3_stmt *stmt;
      bool done;
      bool validRow;

    public:
      Statement(Database &db, const std::string &sql);
      ~Statement();

      bool isDone() const {return done;}
      bool next();
      void reset();
      void clearBindings();

      void execute();
      bool execute(int64_t &result);
      bool execute(double &result);
      bool execute(std::string &result);

      unsigned columns() const;
      /**
       * Result is only valid until either next() is called or the Statement is
       * destructed and if @param i is less than columns().
       *
       * @param i index
       * @return The Column at index @param i.
       */
      Column column(unsigned i) const;

      unsigned parameters() const;
      Parameter parameter(unsigned i) const;
      Parameter parameter(const std::string &name) const;

      // JSON functions
      void readHeader(JSON::Sink &sink);
      void readOne(JSON::Sink &sink);
      void readAll(JSON::Sink &sink);
    };
  }
}
