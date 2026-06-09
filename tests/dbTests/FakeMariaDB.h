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

// A test-only fake of the libmariadb C API.  Linked into a test program in
// place of libmariadbclient so cbang's real DB/EventDB/QueryCallback/Query
// code runs against canned result sets with no database.  The test scripts the
// responses (FakeDB::push) before dispatching, then inspects the captured SQL
// (FakeDB::queries).  Non-blocking calls report "pending" once against an
// always-ready fd, so the real event loop and continue path are exercised.

#include <string>
#include <vector>

namespace FakeDB {
  // MariaDB field type codes (subset) — see cb::MariaDB::Field::type_t.
  enum Type {
    TINY     = 1,
    LONG     = 3,
    DOUBLE   = 5,
    LONGLONG = 8,
    BLOB     = 252,
    STRING   = 253,
  };

  struct Cell {
    bool null = false;
    std::string data;
    Cell(const char *s)        : data(s) {}
    Cell(const std::string &s) : data(s) {}
    Cell()                     : null(true) {} // SQL NULL
  };

  struct Col { std::string name; int type = STRING; };

  struct Result {
    std::vector<Col> cols;
    std::vector<std::vector<Cell>> rows;
  };

  // One query's outcome: zero or more result sets (zero = OK / no result set),
  // or a failure when errnoVal is nonzero.
  struct Response {
    std::vector<Result> results;
    unsigned errnoVal = 0;
    std::string error;
    std::string sqlstate = "00000";
  };

  void reset();
  void push(const Response &r);                // queue one query's outcome
  const std::vector<std::string> &queries();   // SQL sent, in order
  const std::vector<std::vector<std::string>> &binds(); // params, per query
}
