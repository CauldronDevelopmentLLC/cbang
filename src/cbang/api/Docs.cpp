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

#include "Docs.h"

#include <cbang/http/URLPatternMatcher.h>

using namespace std;
using namespace cb;
using namespace cb::API;


namespace {
  void copyExistingKey(const string &key, const JSON::Value &src,
                       JSON::Value &dst) {
    if (src.has(key)) dst.insert(key, src.get(key));
  }


  void copyExistingKeys(const char *keys[], const JSON::Value &src,
                       JSON::Value &dst) {
    for (int i = 0; keys[i]; i++) copyExistingKey(keys[i], src, dst);
  }
}


Docs::Docs(const JSON::ValuePtr &config) {
  insert("title", config->getString("title", "API Docs"));

  const char *keys[] = {"description", "header", "version", 0};
  copyExistingKeys(keys, *config, *this);

  insert("categories", createDict());
}


void Docs::loadCategory(const string &name, const JSON::ValuePtr &config) {
  if (config->getBoolean("hide", false)) {
    category = methods = 0;
    return;
  }

  category = createDict();
  get("categories")->insert(name, category);

  const char *keys[] = {"base", "title", "help", "allow", "deny", 0};
  copyExistingKeys(keys, *config, *category);
}


void Docs::loadEndpoint(const string &path, const JSON::ValuePtr &config) {
  if (category.isNull() || config->getBoolean("hide", false)) {
    methods = 0;
    return;
  }

  if (!category->has("endpoints")) category->insertDict("endpoints");
  auto endpoints = category->get("endpoints");

  // Get URL args from endpoint pattern
  HTTP::URLPatternMatcher matcher(path, new HTTP::RequestHandler);
  urlArgs = matcher.getArgs();
  endpointArgs = config->get("args", createDict());

  methods = createDict();
  endpoints->insert(path, methods);
}


void Docs::loadMethod(const string &method, const string &handler,
                      const JSON::ValuePtr &config) {
  if (methods.isNull() || config->getBoolean("hide", false)) return;

  auto docs = createDict();
  docs->insert("handler", handler);

  // Load args
  auto args = createDict();
  args->merge(*endpointArgs);
  if (config->isDict() && config->has("args"))
    args->merge(*config->get("args"));

  if (args->size()) {
    // Mark URL args
    for (unsigned i = 0; i < args->size(); i++)
      if (urlArgs.find(args->keyAt(i)) != urlArgs.end())
        args->get(i)->insertBoolean("url", true);

    docs->insert("args", args);
  }

  // Copy other keys
  if (config->isDict()) {
    const char *keys[] = {"help", "allow", "deny", 0};
    copyExistingKeys(keys, *config, *docs);
  }

  methods->insert(method, docs);
}
