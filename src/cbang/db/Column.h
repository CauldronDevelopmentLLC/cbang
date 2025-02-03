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

#include "Types.h"

#include <cstdint>


struct sqlite3_stmt;

namespace cb {
  namespace DB {
    class Blob;

    class Column : public Types {
      sqlite3_stmt *stmt;
      unsigned i;

    public:
      Column(sqlite3_stmt *stmt, unsigned i) : stmt(stmt), i(i) {}

      unsigned getIndex() const {return i;}
      type_t getType() const;
      const char *getDeclType() const;
      const char *getName() const;
      const char *getOrigin() const;
      const char *getTableName() const;
      const char *getDBName() const;

      Blob toBlob() const;
      double toDouble() const;
      int64_t toInteger() const;
      bool toBoolean() const;
      const char *toString() const;
      const char *toText() const {return toString();}
    };
  }
}
