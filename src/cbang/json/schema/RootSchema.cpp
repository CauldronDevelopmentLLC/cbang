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

#include "RootSchema.h"

using namespace std;
using namespace cb;
using namespace cb::JSON::Schema;


string RootSchema::resolve(const string &id) const {
  return id; // TODO
}


void RootSchema::set(const string &_id, const SmartPointer<Schema> &schema) {
  auto id = resolve(_id);
  if (!schemas.insert(schemas_t::value_type(id, schema)).second)
    THROW("Schema $id '" << id << "' already exists");
}


const SmartPointer<Schema> &RootSchema::get(const string &_id) const {
  auto id = resolve(_id);
  auto it = schemas.find(id);
  if (it == schemas.end()) THROW("Schema $id '" << id << "' not found");
  return it->second;
}


SmartPointer<Schema> RootSchema::subschema(
  const JSON::Value &spec, const string &name) {
  auto it = spec.find(name);
  return it ? new Schema(*this, **it) : 0;
}


unsigned RootSchema::getUInt(
  const JSON::Value &spec, const string &name, unsigned defVal) {
  auto it = spec.find(name);
  return it ? (int)(*it)->getU32() : defVal;
}
