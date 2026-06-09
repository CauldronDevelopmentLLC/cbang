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
//    "sql": <bool, optional>}
//
// Each key under "context" becomes a resolver namespace.  A string template
// is resolved and printed verbatim; a JSON template is resolved in place and
// printed as compact JSON (so typed substitution is visible).

#include <cbang/Catch.h>
#include <cbang/json/Reader.h>
#include <cbang/json/Value.h>
#include <cbang/api/Resolver.h>

#include <iostream>

using namespace cb;
using namespace cb::API;
using namespace std;


int main(int argc, char *argv[]) {
  try {
    auto input = JSON::Reader::parse(cin);

    Resolver resolver;
    if (input->has("context"))
      for (auto e: input->get("context")->entries())
        resolver.set(e.key(), e.value());

    bool sql  = input->getBoolean("sql", false);
    auto tmpl = input->get("template");

    if (tmpl->isString()) cout << resolver.resolve(tmpl->getString(), sql) << endl;
    else {
      resolver.resolve(*tmpl, sql);
      cout << tmpl->toString(0, true) << endl;
    }

    return 0;
  } CATCH_ERROR;

  return 1;
}
