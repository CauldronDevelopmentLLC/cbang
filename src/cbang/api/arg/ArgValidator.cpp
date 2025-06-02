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

#include "ArgValidator.h"

#include "ArgDict.h"
#include "ArgPattern.h"
#include "ArgEnum.h"
#include "ArgNumber.h"
#include "ArgMinLength.h"
#include "ArgMaxLength.h"
#include "ArgBoolean.h"
#include "ArgAuth.h"
#include "ArgURI.h"

#include <cbang/json/JSON.h>


using namespace cb::API;
using namespace cb;
using namespace std;


#define EMAIL_CHARS "\\w!#$%&*+/=?^`{|}~-"
#define EMAIL_RE \
  "^[" EMAIL_CHARS "][." EMAIL_CHARS "]*@[." EMAIL_CHARS "]*[" EMAIL_CHARS "]$"

#define DATE_RE "\\d{4}-\\d{2}-\\d{2}"
#define TIME_RE "\\d{2}:\\d{2}:\\d{2}(\\.\\d{1,6})?"
#define ZONE_RE "(Z|([+-]\\d{2}:\\d{2}))"
#define ISO8601_RE DATE_RE "T" TIME_RE ZONE_RE


ArgValidator::ArgValidator(const JSON::ValuePtr &config) :
  config(config), defaultValue(config->get("default", 0)) {

  optional = config->getBoolean("optional", defaultValue.isSet());
  if (!optional && defaultValue.isSet())
    THROW("It does not make sense for a required option to have a default");

  type = config->getString("type", "");

  // Implicit types
  if (type.empty()) {
    if      (config->has("dict")) type = "dict";
    else if (config->has("enum")) type = "enum";
    else                          type = "string";

    config->insert("type", type);
  }

  if      (type == "dict")   add(new ArgDict(config->get("dict")));
  else if (type == "enum")   add(new ArgEnum(config));
  else if (type == "number") add(new ArgNumber<double>(config));
  else if (type == "float")  add(new ArgNumber<float>(config));
  else if (type == "int")    add(new ArgNumber<int64_t>(config));
  else if (type == "s64")    add(new ArgNumber<int64_t>(config));
  else if (type == "u64")    add(new ArgNumber<uint64_t>(config));
  else if (type == "s32")    add(new ArgNumber<int32_t>(config));
  else if (type == "u32")    add(new ArgNumber<uint32_t>(config));
  else if (type == "s16")    add(new ArgNumber<int16_t>(config));
  else if (type == "u16")    add(new ArgNumber<uint16_t>(config));
  else if (type == "s8")     add(new ArgNumber<int8_t>(config));
  else if (type == "u8")     add(new ArgNumber<uint8_t>(config));
  else if (type == "bool")   add(new ArgBoolean);
  else if (type == "email")  add(new ArgPattern(EMAIL_RE));
  else if (type == "time")   add(new ArgPattern(ISO8601_RE));
  else if (type == "date")   add(new ArgPattern(DATE_RE));
  else if (type == "uri")    add(new ArgURI);
  else if (type != "string")
    THROW("Unsupported argument type '" << type << "'");

  if (type == "string" || type == "email" || type == "uri") {
    if (config->hasNumber("min")) add(new ArgMinLength(config));
    if (config->hasNumber("max")) add(new ArgMaxLength(config));
  }

  if (config->has("pattern")) add(new ArgPattern(config->getString("pattern")));
  if (config->has("allow"))   add(new ArgAuth(true,  config->get("allow")));
  if (config->has("deny"))    add(new ArgAuth(false, config->get("deny")));
}


JSON::ValuePtr ArgValidator::getSpec() const {
  JSON::ValuePtr spec = new JSON::Dict;

  if (config->hasString("help"))
    spec->insert("description", config->get("help"));
  if (!isOptional()) spec->insertBoolean("required", true);

  spec->insert("schema", getSchema());

  return spec;
}


JSON::ValuePtr ArgValidator::getSchema() const {
  JSON::ValuePtr schema = new JSON::Dict;

  addSchema(*schema);

  if (type == "email") {
    schema->insert("type", "string");
    schema->insert("format", "email");
    schema->insert("pattern", EMAIL_RE);

  } else if (type == "time") {
    schema->insert("type", "string");
    schema->insert("format", "date-time");
    schema->insert("pattern", ISO8601_RE);

  } else if (type == "date") {
    schema->insert("type", "string");
    schema->insert("format", "date");
    schema->insert("pattern", DATE_RE);

  } else if (type == "uri") {
    schema->insert("type", "string");
    schema->insert("format", "uri");

  } else if (type == "string")
    schema->insert("type", "string");

  return schema;
}


void ArgValidator::add(const SmartPointer<ArgConstraint> &constraint) {
  constraints.push_back(constraint);
}


void ArgValidator::operator()(const ResolverPtr &resolver, JSON::Value &value) const {
  for (auto &c: constraints) (*c)(resolver, value);
}


void ArgValidator::addSchema(JSON::Value &schema) const {
  for (auto &c: constraints) c->addSchema(schema);
}
