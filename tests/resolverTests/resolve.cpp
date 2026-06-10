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

// Test driver for cb::API::Resolver.  Reads a JSON document on stdin:
//
//   {"context": {"args": {...}, "options": {...}, ...},
//    "template": <string or JSON value>,
//    "sql": <bool, optional>, "partial": <bool, optional>}
//
// Each key under "context" becomes a resolver namespace.  With "sql" the
// template is resolved as SQL and the bound parameters are printed as
// PARAM[i] lines.  Otherwise a string template is resolved and printed
// verbatim and a JSON template is resolved in place and printed as compact
// JSON (so typed substitution is visible).  "partial" leaves missing refs
// unresolved.

#include <cbang/Catch.h>
#include <cbang/json/Reader.h>
#include <cbang/json/Value.h>
#include <cbang/api/Resolver.h>
#include <cbang/log/Logger.h>

#include <iostream>

using namespace cb;
using namespace cb::API;
using namespace std;


int main(int argc, char *argv[]) {
  try {
    // Deterministic logs
    Logger::instance().setLogTime(false);
    Logger::instance().setLogColor(false);
    Exception::printLocations = false;

    auto input = JSON::Reader::parse(cin);

    Resolver resolver;
    if (input->has("context"))
      for (auto e: input->get("context")->entries())
        resolver.set(e.key(), e.value());

    bool sql     = input->getBoolean("sql", false);
    bool partial = input->getBoolean("partial", false);
    auto tmpl    = input->get("template");

    if (sql) {
      vector<JSON::ValuePtr> params;
      cout << resolver.resolveSQL(tmpl->getString(), params) << endl;
      for (unsigned i = 0; i < params.size(); i++)
        cout << "PARAM[" << i << "]: " << params[i]->toString(0, true) << endl;

    } else if (tmpl->isString())
      cout << resolver.resolve(tmpl->getString(), partial) << endl;

    else {
      resolver.resolve(*tmpl, partial);
      cout << tmpl->toString(0, true) << endl;
    }

    return 0;
  } CATCH_ERROR;

  return 1;
}
