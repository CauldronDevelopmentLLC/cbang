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

#include "Attributes.h"

#include <cbang/String.h>

#include <cctype>

using namespace std;
using namespace cb;
using namespace cb::XML;


Attributes::Attributes(const string &attrs) {
  string name;
  string value;
  unsigned state = 0;
  unsigned count = 0;

  auto it = attrs.begin();
  while (true) {
    if (it == attrs.end()) {
      if (state == 0) break;
      if (state != 6) THROW("Incomplete attribute definition");
    }

    switch (state) {
    case 0: // Start
      if (!isspace(*it)) {state = 1; continue;}
      break;

    case 1: // Name
      if (*it == '=') state = 3;
      else if (isspace(*it)) state = 2;
      else name.append(1, *it);
      break;

    case 2: // Before equal
      if (*it == '=') state = 3;
      else if (!isspace(*it)) {state = 3; continue;}
      break;

    case 3: // Before value
      if (name.empty())
        THROW("Empty name in attribute definition at " << count);

      if (*it == '"') state = 4;
      else if (*it == '\'') state = 5;
      else if (!isspace(*it))
        THROW("Expected ' or \" in attribute definition at " << count);
      break;

    case 4: // " Value
      if (*it == '"') state = 6;
      else value.append(1, *it);
      break;

    case 5: // ' Value
      if (*it == '\'') state = 6;
      else value.append(1, *it);
      break;

    case 6:
      set(name, value);
      name.clear();
      value.clear();
      state = 0;
      continue;
    }

    count++;
    it++;
  }
}


const string Attributes::toString() const {
  vector<string> attrs;
  for (auto &p: *this)
    attrs.push_back(p.first + "=" + p.second);

  return String::join(attrs);
}
