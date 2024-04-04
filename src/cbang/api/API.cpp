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

#include "API.h"

#include <cbang/api/handler/ArgsParser.h>
#include <cbang/api/handler/StatusHandler.h>
#include <cbang/api/handler/QueryHandler.h>
#include <cbang/api/handler/LoginHandler.h>
#include <cbang/api/handler/LogoutHandler.h>
#include <cbang/api/handler/APIHandler.h>
#include <cbang/api/handler/ArgsHandler.h>
#include <cbang/api/handler/ArgFilterHandler.h>
#include <cbang/api/handler/PassHandler.h>
#include <cbang/api/handler/HeadersHandler.h>
#include <cbang/api/handler/RedirectHandler.h>
#include <cbang/api/handler/SessionHandler.h>
#include <cbang/api/handler/CORSHandler.h>

#include <cbang/http/Request.h>
#include <cbang/http/HandlerGroup.h>
#include <cbang/http/URLPatternMatcher.h>
#include <cbang/http/MethodMatcher.h>
#include <cbang/http/AccessHandler.h>
#include <cbang/http/FileHandler.h>
#include <cbang/http/IndexHandler.h>

#include <cbang/openssl/SSLContext.h>
#include <cbang/json/Value.h>
#include <cbang/json/String.h>
#include <cbang/log/Logger.h>
#include <cbang/config/Options.h>

using namespace cb;
using namespace std;
using namespace cb::API;


namespace {
  unsigned parseMethods(const std::string &s) {
    unsigned methods = HTTP::Method::HTTP_UNKNOWN;
    vector<string> tokens;

    String::tokenize(String::toUpper(s), tokens, "| \n\r\t");
    for (unsigned i = 0; i < tokens.size(); i++)
      methods |= HTTP::Method::parse(
        tokens[i], HTTP::Method::HTTP_UNKNOWN);

    return methods;
  }
}


string cb::API::API::getEndpointType(const JSON::ValuePtr &config) const {
  string type = config->getString("handler", "");

  if (type.empty()) {
    if (config->has("sql")) return "query";
    return "pass";
  }

  return type;
}


SmartPointer<HTTP::RequestHandler>
cb::API::API::createAccessHandler(const JSON::ValuePtr &config) {
  auto handler = SmartPtr(new HTTP::AccessHandler);

  if (config->has("allow")) {
    JSON::ValuePtr list = config->get("allow");
    for (unsigned i = 0; i < list->size(); i++) {
      string name = list->getAsString(i);
      if (name.empty()) continue;
      if (name[0] == '$') handler->allowGroup(name.substr(1));
      else handler->allowUser(name);
    }
  }

  if (config->has("deny")) {
    JSON::ValuePtr list = config->get("deny");
    for (unsigned i = 0; i < list->size(); i++) {
      string name = list->getAsString(i);
      if (name.empty()) continue;
      if (name[0] == '$') handler->denyGroup(name.substr(1));
      else handler->denyUser(name);
    }
  }

  return handler;
}


SmartPointer<HTTP::RequestHandler>
cb::API::API::createEndpoint(const JSON::ValuePtr &config) {
  string type = getEndpointType(config);

  if (type == "pass")     return new PassHandler;
  if (type == "cors")     return new CORSHandler(config);
  if (type == "status")   return new StatusHandler(config);
  if (type == "redirect") return new RedirectHandler(config);
  if (type == "api")      return new APIHandler(config, this->config);

  if (client.isSet() && oauth2Providers.isSet() && sessionManager.isSet()) {
    if (config->hasString("sql") && connector.isNull())
      THROW("Cannot have 'sql' in API without a DB connector");

    if (type == "login")   return new LoginHandler  (*this, config);
    if (type == "logout")  return new LogoutHandler (*this, config);
    if (type == "session") return new SessionHandler(*this, config);
  }

  if (connector.isSet() && type == "query")
    return new QueryHandler(*this, config);

  THROW("Unsupported handler '" << type << "'");
}


SmartPointer<HTTP::RequestHandler>
cb::API::API::createValidationHandler(const JSON::ValuePtr &config) {
  auto group = SmartPtr(new HTTP::HandlerGroup);

  // Endpoint Auth
  if (config->has("allow") || config->has("deny"))
    group->addHandler(createAccessHandler(config));

  // Args
  if (config->has("args"))
    group->addHandler(new ArgsHandler(config->get("args")));

  return group;
}


SmartPointer<HTTP::RequestHandler>
cb::API::API::createAPIHandler(const string &pattern, const JSON::ValuePtr &api,
                         const RequestHandlerPtr &parentValidation) {
  // Patterns
  auto patterns = SmartPtr(new HTTP::HandlerGroup);

  // Group
  auto group = SmartPtr(new HTTP::HandlerGroup);
  auto root  = SmartPtr(new HTTP::URLPatternMatcher(pattern, group));
  patterns->addHandler(root);

  // Request validation
  auto validation = group->addGroup();
  if (parentValidation.isSet()) group->addHandler(parentValidation);
  validation->addHandler(createValidationHandler(api));

  // Children
  for (unsigned i = 0; i < api->size(); i++) {
    auto &key    = api->keyAt(i);
    auto &config = api->get(i);

    // Child
    if (!key.empty() && key[0] == '/') {
      auto child = createAPIHandler(pattern + key, config, validation);
      patterns->addHandler(child);
      continue;
    }

    // Methods
    unsigned methods = parseMethods(key);
    if (!methods) continue; // Ignore non-methods

    LOG_INFO(3, "Adding endpoint " << key << " " << pattern
             << " (" << getEndpointType(config) << ")");

    auto handler = createEndpoint(config);
    if (handler.isNull()) continue;

    auto methodGroup = SmartPtr(new HTTP::HandlerGroup);
    methodGroup->addHandler(createValidationHandler(config));

    // Headers
    if (config->has("headers") && !handler.isInstance<HeadersHandler>())
      methodGroup->addHandler(new HeadersHandler(config->get("headers")));

    // Arg filter
    if (config->has("arg-filter")) {
      if (procPool.isNull())
        THROW("API cannot have 'arg-filter' without Event::SubprocessPool");

      handler = new ArgFilterHandler(*this, config->get("arg-filter"), handler);
    }

    // Method(s)
    methodGroup->addHandler(handler);
    group->addHandler(new HTTP::MethodMatcher(methods, methodGroup));
  }

  return patterns;
}


void cb::API::API::loadCategory(const string &name, const JSON::ValuePtr &cat) {
  auto group = SmartPtr(new HTTP::HandlerGroup);
  string base = cat->getString("base", "");

  // Category Auth
  if (cat->has("allow") || cat->has("deny"))
    group->addHandler(createAccessHandler(cat));

  // Endpoints
  if (cat->hasDict("endpoints")) {
    auto &endpoints = *cat->get("endpoints");

    for (unsigned i = 0; i < endpoints.size(); i++)
      group->addHandler(
        createAPIHandler(base + endpoints.keyAt(i), endpoints.get(i)));
  }

  addHandler(group);
}


void cb::API::API::loadCategories(const JSON::ValuePtr &config) {
  for (unsigned i = 0; i < config->size(); i++)
    loadCategory(config->keyAt(i), config->get(i));
}


void cb::API::API::load(const JSON::ValuePtr &config) {
  if (this->config.isSet()) THROW("API already loaded");
  this->config = config;

  // Check version
  const Version minVersion("1.0.0");
  Version version(config->getString("version", "0.0.0"));
  if (version < minVersion)
    THROW("API version must be at least " << minVersion);

  // Always parse args
  addHandler(new ArgsParser);

  if (config->has("api")) loadCategories(config->get("api"));
}
