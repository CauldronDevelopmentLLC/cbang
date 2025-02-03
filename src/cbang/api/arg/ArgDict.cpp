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

#include "ArgDict.h"

using namespace cb;
using namespace std;
using namespace cb::API;


void ArgDict::add(const JSON::ValuePtr &args) {
  for (auto e: args->entries())
    validators[e.key()] = new ArgValidator(e.value());
}


void ArgDict::appendSpecs(JSON::Value &spec) const {
  for (auto &p: validators) {
    auto argSpec = p.second->getSpec();
    argSpec->insert("name", p.first);
    spec.append(argSpec);
  }
}


void ArgDict::operator()(HTTP::Request &req, JSON::Value &value) const {
  if (!value.isDict()) THROWX("Invalid arguments", HTTP_BAD_REQUEST);

  set<string> found;

  for (auto e: value.entries()) {
    const string &name = e.key();

    found.insert(name);

    auto it2 = validators.find(name);
    if (it2 == validators.end()) continue; // Ignore unrecognized args

    try {
      (*it2->second)(req, *e);

    } catch (const Exception &ex) {
      if (ex.getCode() == HTTP_UNAUTHORIZED)
        THROWX("Access denied", HTTP_UNAUTHORIZED);

      THROWX("Invalid argument '" << name << "=" << e->asString()
              << "': " << ex.getMessage(), HTTP_BAD_REQUEST);
    }
  }

  // Make sure all required arguments were found and handle defaults
  vector<string> missing;
  for (auto &p: validators)
    if (found.find(p.first) == found.end()) {
      const ArgValidator &av = *p.second;

      if (av.hasDefault()) value.insert(p.first, av.getDefault());
      else if (!av.isOptional()) missing.push_back(p.first);
    }

  if (!missing.empty())
    THROWX("Missing argument" << (1 < missing.size() ? "s" : "") << ": "
            << String::join(missing, ", "), HTTP_BAD_REQUEST);
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
