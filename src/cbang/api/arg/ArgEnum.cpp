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

#include "ArgEnum.h"

#include <cbang/json/List.h>

using namespace cb::API;
using namespace cb;
using namespace std;


ArgEnum::ArgEnum(const JSON::ValuePtr &config) :
  caseSensitive(config->getBoolean("case-sensitive", false)) {
  auto list = config->get("enum");

  for (auto &value: *list) {
    string s = value->getString();
    values.insert(caseSensitive ? s : String::toLower(s));
  }
}


void ArgEnum::operator()(const ResolverPtr &resolver, JSON::Value &_value) const {
  if (!_value.isString()) THROW("Enum argument must be string");

  string value = _value.getString();
  if (caseSensitive) value = String::toLower(value);

  if (values.find(value) == values.end())
    THROW("Must be one of: " << String::join(values, ", "));
}


void ArgEnum::addSchema(JSON::Value &schema) const {
  schema.insert("type", "string");

  JSON::ValuePtr list = new JSON::List;
  for (auto &v: values) list->append(v);
  schema.insert("enum", list);
}
