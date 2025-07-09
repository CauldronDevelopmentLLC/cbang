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

#include "ArgDict.h"

#include <cbang/api/API.h>

using namespace cb;
using namespace std;
using namespace cb::API;


void ArgDict::load(const JSON::ValuePtr &def) {
  if (def->isString()) load(api.getArgs(def->getString()));

  else if (def->isList())
    for (auto &e: *def)
      load(e);

  else if (def->isDict())
    for (auto e: def->entries())
      add(e.key(), e.value());

  else THROW("Invalid arg def type " << def->getType());
}


void ArgDict::add(const string &name, const JSON::ValuePtr &_arg) {
  JSON::ValuePtr arg = _arg;
  set<string> inherited;

  if (!arg->isDict()) THROW("Arg def is not a dictionary: " << arg->toString());

  while (arg->hasString("inherit")) {
    auto parentName = arg->getString("inherit");
    if (!inherited.insert(parentName).second)
      THROW("Recursive arg inheritance with arg '" << parentName << "'");

    auto parent = api.getArgs(parentName);
    auto tmp    = arg->copy();
    tmp->erase("inherit");
    tmp->merge(*parent);
    arg = tmp;
  }

  validators[name] = new ArgValidator(api, arg);
}


void ArgDict::appendSpecs(JSON::Value &spec) const {
  for (auto &p: validators) {
    auto argSpec = p.second->getSpec();
    argSpec->insert("name", p.first);
    spec.append(argSpec);
  }
}


JSON::ValuePtr ArgDict::operator()(
  const CtxPtr &ctx, const JSON::ValuePtr &args) const {

  if (!args->isDict()) THROWX("Invalid arguments", HTTP_BAD_REQUEST);

  JSON::ValuePtr result = new JSON::Dict;
  vector<string> errors;

  for (auto &p: validators) {
    const ArgValidator &av = *p.second;
    auto name   = p.first;
    auto source = av.hasSource() ? av.getSource() : name;
    auto value  = args->get(source, 0);

    if (value.isNull()) {
      if (av.hasDefault()) result->insert(name, av.getDefault());
      else if (!av.isOptional())
        errors.push_back(SSTR("Missing argument '" << source << "'"));

    } else
      try {
        result->insert(name, av(ctx, value));

      } catch (const Exception &ex) {
        string msg = SSTR("Invalid argument '" << source << "'='"
          << value->asString() << "' " << ex.getMessages());

        if (ex.getCode() && ex.getCode() != HTTP_BAD_REQUEST)
          THROWX(msg, ex.getCode());

        errors.push_back(msg);
      }
  }

  if (!errors.empty()) THROWX(String::join(errors, ", "), HTTP_BAD_REQUEST);

  return result;
}


void ArgDict::addSchema(JSON::Value &schema) const {
  schema.insert("type", "object");

  auto props = schema.createDict();
  schema.insert("properties", props);

  for (auto &p: validators) {
    auto propSchema = schema.createDict();
    props->insert(p.first, propSchema);
    p.second->addSchema(*propSchema);
  }
}
