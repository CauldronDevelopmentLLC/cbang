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

#include "Array.h"
#include "RootSchema.h"

#include <limits>

using namespace std;
using namespace cb;
using namespace cb::JSON::Schema;


namespace {
  struct CompareValuePtrs {
    bool operator()(const JSON::ValuePtr &a, const JSON::ValuePtr &b) const {
      return *a < *b;
    }
  };
}


Array::Array(RootSchema &root, const JSON::Value &spec) :
  maxContains(
    root.getUInt(spec, "maxContains", numeric_limits<unsigned>::max())),
  minContains(root.getUInt(spec, "minContains", 1)),
  maxItems(root.getUInt(spec, "maxItems", numeric_limits<unsigned>::max())),
  minItems(root.getUInt(spec, "minItems", 0)) {

  if (spec.has("unevaluatedItems"))
    THROW("JSON Schema keyword 'unevaluatedItems' not supported");

  auto uniqueIt = spec.find("uniqueItems");
  if (uniqueIt) unique = (*uniqueIt)->getBoolean();

  items    = root.subschema(spec, "items");
  contains = root.subschema(spec, "contains");

  auto prefixIt = spec.find("prefixItems");
  if (prefixIt)
    for (auto schema: **prefixIt)
      prefix.push_back(new Schema(root, *schema));
}


bool Array::match(const JSON::Value &values) const {
  if (!values.isList()) return false;

  // Length
  if (values.size() < minItems) return false;
  if (maxItems < values.size()) return false;

  unsigned i = 0;

  // Prefix
  if (values.size() < prefix.size()) return false;
  for (; i < prefix.size(); i++)
    if (!prefix[i]->match(*values.get(i))) return false;

  // Items
  for (; items.isSet() && i < values.size(); i++)
    if (!items->match(*values.get(i))) return false;

  // Contains
  if (contains.isSet()) {
    unsigned count = 0;

    for (unsigned j = 0; j < values.size(); j++) {
      if (contains->match(*values.get(j))) count++;
      if (maxContains <  count) return false;
      if (minContains <= count) break;
    }

    if (count < minContains) return false;
  }

  // Unique
  if (unique) {
    set<ValuePtr, CompareValuePtrs> unique;
    for (auto value: values)
      if (!unique.insert(value).second)
        return false;
  }

  return true;
}
