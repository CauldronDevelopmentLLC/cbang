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

#include "Schema.h"
#include "RootSchema.h"
#include "False.h"
#include "Boolean.h"
#include "Null.h"
#include "Number.h"
#include "Array.h"
#include "Object.h"
#include "Enum.h"
#include "Const.h"
#include "String.h"
#include "AllOf.h"
#include "AnyOf.h"
#include "OneOf.h"
#include "Not.h"
#include "DependentSchemas.h"
#include "DependentRequired.h"
#include "IfThenElse.h"

using namespace std;
using namespace cb;
using namespace cb::JSON::Schema;


Schema::Schema(RootSchema &root, const JSON::Value &spec) :
  Schema(root) {parse(spec);}


bool Schema::match(const JSON::Value &data) const {
  for (auto &constraint: constraints)
    if (!constraint->match(data)) return false;

  return true;
}


void Schema::parse(const JSON::Value &spec) {
  // $id
  auto idIt = spec.find("$id");
  if (idIt) root.set((*idIt)->getString(), this);

  // boolean
  if (spec.isBoolean()) {
    if (!spec.getBoolean()) add(new False);
    return;
  }

  // type
  auto typeIt = spec.find("type");
  if (typeIt) {
    auto &type = **typeIt;

    if (type.isString()) add(parseType(type.getString(), spec));
    else if (type.isList()) add(SmartPtr(new AnyOf(root, type)));

    else THROW("Invalid JSON Schema type '" << type << "'");
  }

  // enum
  auto enumIt = spec.find("enum");
  if (enumIt) add(new Enum(**enumIt));

  // const
  auto constIt = spec.find("const");
  if (constIt) add(new Const(*constIt));

  // allOf
  auto allOfIt = spec.find("allOf");
  if (allOfIt) add(new AllOf(root, **allOfIt));

  // anyOf
  auto anyOfIt = spec.find("anyOf");
  if (anyOfIt) add(new AnyOf(root, **anyOfIt));

  // oneOf
  auto oneOfIt = spec.find("oneOf");
  if (oneOfIt) add(new OneOf(root, **oneOfIt));

  // not
  auto notIt = spec.find("not");
  if (notIt) add(new Not(root, **notIt));

  // dependentSchemas
  auto depSchemasIt = spec.find("dependentSchemas");
  if (depSchemasIt) add(new DependentSchemas(root, **depSchemasIt));

  // dependentRequired
  auto depReqIt = spec.find("dependentRequired");
  if (depReqIt) add(new DependentRequired(**depReqIt));

  // if, then, else
  auto ifIt = spec.find("if");
  if (ifIt) add(new IfThenElse(root, spec));

  // The following keywords of JSON Schema (Draft 2019-09) are not implemented:
  //   contentEncoding, contentMediaType, contentSchema
  //   $id, $anchor, $ref, $defs, $dynamicAnchor, $dynamicRef
  //   $schema, $vocabulary, unevaluatedProperties, unevaluatedItems

  // The following annotation and comment keywords are ignored:
  //  title, description, default, examples, deprecated, readOnly, writeOnly
  //  $comment
}


ConstraintPtr Schema::parseType(const string &type, const JSON::Value &spec) {
  if (type == "array")   return new Array(root, spec);
  if (type == "boolean") return new Boolean;
  if (type == "null")    return new Null;
  if (type == "integer") return new Number(spec);
  if (type == "number")  return new Number(spec);
  if (type == "object")  return new Object(root, spec);
  if (type == "string")  return new String(spec);
  THROW("Unsupported JSON Schema type '" << type << "'");
}
