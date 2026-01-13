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

#include "Object.h"
#include "RootSchema.h"

#include <cbang/json/String.h>

#include <limits>

using namespace std;
using namespace cb;
using namespace cb::JSON::Schema;


Object::Object(RootSchema &root, const JSON::Value &spec) :
  maxProps(
    root.getUInt(spec, "maxProperties", numeric_limits<unsigned>::max())),
  minProps(root.getUInt(spec, "minProperties", 0)) {
  if (spec.has("unevaluatedProperties"))
    THROW("JSON Schema keyword 'unevaluatedProperties' not supported");

  auto propsIt = spec.find("properties");
  if (propsIt)
    for (auto e: (*propsIt)->entries())
      props[e.key()] = new Schema(root, *e.value());

 auto patPropsIt = spec.find("patternProperties");
  if (patPropsIt)
    for (auto e: (*propsIt)->entries())
      patternProps.push_back({e.key(), new Schema(root, *e.value())});

  additional = root.subschema(spec, "additionalProperties");
  names      = root.subschema(spec, "propertyNames");

  auto reqIt = spec.find("required");
  if (reqIt)
    for (auto &name: (*reqIt)->getList())
      required.insert(name->getString());
}


bool Object::match(const JSON::Value &value) const {
  if (!value.isDict()) return false;
  if (maxProps < value.size() || value.size() < minProps) return false;

  for (auto name: required)
    if (!value.has(name)) return false;

  for (auto e: value.entries()) {
    bool matched = false;

    if (names.isSet() && !names->match(JSON::String(e.key()))) return false;

    auto it = props.find(e.key());
    if (it != props.end()) {
      if (it->second->match(*e.value())) matched = true;
      else return false;
    }

    for (auto &pp: patternProps)
      if (pp.re.match(e.key())) {
        if (pp.schema->match(*e.value())) matched = true;
        else return false;
      }

    if (!matched && additional.isSet() && !additional->match(*e.value()))
      return false;
  }

  return true;
}
