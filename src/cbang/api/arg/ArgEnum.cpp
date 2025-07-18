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
  values(config->get("enum")) {
  if (!values->isList()) THROW("Arg enum must be list");
}


JSON::ValuePtr ArgEnum::operator()(
  const CtxPtr &ctx, const JSON::ValuePtr &value) const {

  for (auto v: *values)
    if (*v == *value) return value;

  THROW("Must be one of: " << values->toString());
}


void ArgEnum::addSchema(JSON::Value &schema) const {
  schema.insert("type", "string");

  JSON::ValuePtr list = new JSON::List;
  for (auto v: *values) list->append(v);
  schema.insert("enum", list);
}
