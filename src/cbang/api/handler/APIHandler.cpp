/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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

#include "APIHandler.h"

#include <cbang/String.h>
#include <cbang/http/Request.h>
#include <cbang/http/URLPatternMatcher.h>
#include <cbang/http/Method.h>

using namespace cb::API;
using namespace cb;
using namespace std;


namespace {
  void copyExistingKey(const string &key, const JSON::Value &src,
                       JSON::Value &dst) {
    if (src.has(key)) dst.insert(key, src.get(key));
  }


  void copyExistingKeys(const char *keys[], const JSON::Value &src,
                       JSON::Value &dst) {
    for (int i = 0; keys[i]; i++) copyExistingKey(keys[i], src, dst);
  }


  bool isMethod(const std::string &s) {
    vector<string> tokens;

    String::tokenize(String::toUpper(s), tokens, "| \n\r\t");
    for (unsigned i = 0; i < tokens.size(); i++)
      if (HTTP::Method::parse(
            tokens[i], HTTP::Method::HTTP_UNKNOWN) ==
          HTTP::Method::HTTP_UNKNOWN) return false;

    return !tokens.empty();
  }
}


APIHandler::APIHandler(
  const JSON::ValuePtr &config, const JSON::ValuePtr &apiConfig) :
  api(new JSON::Dict) {

  api->insert("title", config->getString("title", "API Docs"));
  const char *keys[] = {"description", "header", 0};
  copyExistingKeys(keys, *config, *api);
  api->insert("categories", loadCategories(*apiConfig->get("api")));
}


bool APIHandler::operator()(HTTP::Request &req) {
  api->write(*req.getJSONWriter());
  req.reply();
  return true;
}


JSON::ValuePtr APIHandler::loadCategories(const JSON::Value &_cats) {
  JSON::ValuePtr cats = new JSON::Dict;

  for (unsigned i = 0; i < _cats.size(); i++) {
    JSON::Value &cat = *_cats.get(i);
    if (!cat.getBoolean("hide", false))
      cats->insert(_cats.keyAt(i), loadCategory(cat));
  }

  return cats;
}


JSON::ValuePtr APIHandler::loadCategory(const JSON::Value &_cat) {
  JSON::ValuePtr cat = new JSON::Dict;

  string base = _cat.getString("base", "");
  const char *keys[] = {"base", "title", "help", "allow", "deny", 0};
  copyExistingKeys(keys, _cat, *cat);

  if (_cat.hasDict("endpoints")) {
    const JSON::Value &_endpoints = *_cat.get("endpoints");
    JSON::ValuePtr endpoints = new JSON::Dict;

    cat->insert("endpoints", endpoints);

    for (unsigned i = 0; i < _endpoints.size(); i++) {
      string pattern = base + _endpoints.keyAt(i);
      const JSON::Value &_endpoint = *_endpoints.get(i);
      if (_endpoint.getBoolean("hide", false)) continue;

      loadEndpoints(endpoints, pattern, _endpoint);
    }
  }

  return cat;
}


void APIHandler::loadEndpoints(
  const JSON::ValuePtr &endpoints, const string &pattern,
  const JSON::Value &config) {
  JSON::ValuePtr methods = new JSON::Dict;
  endpoints->insert(pattern, methods);

  // TODO apply "help", "allow" and "deny" to child endpoints

  // Get URL args from endpoint pattern
  HTTP::URLPatternMatcher matcher(pattern, new HTTP::RequestHandler);
  const set<string> &urlArgs = matcher.getArgs();
  JSON::ValuePtr endpointArgs = config.get("args", new JSON::Dict);

  // Load methods
  for (unsigned i = 0; i < config.size(); i++) {
    const auto &key     = config.keyAt(i);
    const auto &_config = *config.get(i);

    if (!_config.isDict()) continue;

    // Hide some handlers in docs
    string handler = _config.getString("handler", "");
    bool hide = handler == "pass" || handler == "session";
    if (_config.getBoolean("hide", hide)) continue;

    // TODO include parent pattern args
    if (!key.empty() && key[0] == '/')
      loadEndpoints(endpoints, pattern + key, _config);

    // Ignore non-methods
    else if (isMethod(key))
      methods->insert(key, loadMethod(_config, urlArgs, *endpointArgs));
  }
}


JSON::ValuePtr APIHandler::loadMethod(const JSON::Value &_method,
                                      const set<string> &urlArgs,
                                      const JSON::Value &endpointArgs) {
  JSON::ValuePtr method = new JSON::Dict;

  // Load args
  JSON::ValuePtr args = new JSON::Dict;
  args->merge(endpointArgs);
  if (_method.has("args")) args->merge(*_method.get("args"));
  if (args->size()) {
    // Mark URL args
    for (unsigned i = 0; i < args->size(); i++)
      if (urlArgs.find(args->keyAt(i)) != urlArgs.end())
        args->get(i)->insertBoolean("url", true);

    method->insert("args", args);
  }

  // Copy other keys
  const char *keys[] = {"help", "handler", "allow", "deny", 0};
  copyExistingKeys(keys, _method, *method);

  return method;
}
