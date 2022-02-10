/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#include "Path.h"
#include "Value.h"

#include <cbang/String.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::JSON;


Path::Path(const string &path) : path(path) {
  String::tokenize(path, parts, ".");
  if (path.empty()) THROW("JSON Path cannot be empty");
}


ValuePtr Path::select(const Value &value, fail_cb_t fail_cb) const {
  ValuePtr ptr;
  unsigned i;

  for (i = 0; i < parts.size(); i++) {
    const Value &v = i ? *ptr : value;

    int index = -1;

    if (v.isList())
      try {
        index = String::parseU32(parts[i], true);
      } catch (const Exception &e) {}

    else if (v.isDict()) index = v.indexOf(parts[i]);

    if (index == -1 || (int)v.size() <= index) break;
    ptr = v.get(index);
  }

  if (i == parts.size()) return ptr;

  string path =
    cb::String::join(vector<string>(parts.begin(), parts.begin() + i + 1), ".");

  if (fail_cb) return fail_cb(path);
  CBANG_KEY_ERROR("At JSON path: " << path);
}


ValuePtr Path::select(const Value &value, const ValuePtr &defaultValue) const {
  auto cb = [&] (const string &path) {return defaultValue;};
  return select(value, cb);
}


#define CBANG_JSON_VT(NAME, TYPE)                                   \
  TYPE Path::select##NAME(const Value &value) const {               \
    ValuePtr result = select(value);                                \
                                                                    \
    if (!result->is##NAME())                                        \
      CBANG_TYPE_ERROR("Not a " #NAME " at " << path);              \
                                                                    \
    return result->get##NAME();                                     \
  }
#include "ValueTypes.def"


#define CBANG_JSON_VT(NAME, TYPE)                                   \
  TYPE Path::select##NAME(const Value &value,                       \
                          TYPE defaultValue) const {                \
    ValuePtr result = select(value, ValuePtr());                    \
                                                                    \
    if (result.isNull() || !result->is##NAME())                     \
      return defaultValue;                                          \
                                                                    \
    return result->get##NAME();                                     \
  }
#include "ValueTypes.def"
