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

#include "API.h"

#include "Resolver.h"
#include "QueryDef.h"

#include <cbang/api/handler/ArgsHandler.h>
#include <cbang/api/handler/ArgsParser.h>
#include <cbang/api/handler/StatusHandler.h>
#include <cbang/api/handler/QueryHandler.h>
#include <cbang/api/handler/LoginHandler.h>
#include <cbang/api/handler/LogoutHandler.h>
#include <cbang/api/handler/SpecHandler.h>
#include <cbang/api/handler/ArgFilterHandler.h>
#include <cbang/api/handler/PassHandler.h>
#include <cbang/api/handler/HeadersHandler.h>
#include <cbang/api/handler/RedirectHandler.h>
#include <cbang/api/handler/SessionHandler.h>
#include <cbang/api/handler/CORSHandler.h>
#include <cbang/api/handler/TimeseriesHandler.h>
#include <cbang/api/handler/WebsocketHandler.h>

#include <cbang/http/Request.h>
#include <cbang/http/HandlerGroup.h>
#include <cbang/http/URLPatternMatcher.h>
#include <cbang/http/MethodMatcher.h>
#include <cbang/http/FileHandler.h>
#include <cbang/http/ResourceHandler.h>

#include <cbang/config/Options.h>
#include <cbang/openssl/SSLContext.h>
#include <cbang/json/Value.h>
#include <cbang/log/Logger.h>

using namespace cb;
using namespace std;
using namespace cb::API;


namespace {
  unsigned parseMethods(const string &s) {
    unsigned methods = HTTP::Method::HTTP_UNKNOWN;
    vector<string> tokens;

    String::tokenize(s, tokens, "|");
    for (auto &token: tokens)
      methods |= HTTP::Method::parse(token, HTTP::Method::HTTP_UNKNOWN);

    return methods;
  }
}


cb::API::API::API(Options &options) : options(options) {}
cb::API::API::~API() {}


void cb::API::API::load(const JSON::ValuePtr &config) {
  if (this->config.isSet()) THROW("API already loaded");
  this->config = config;

  optionValues = options.getConfigJSON();

  // Resolve variables
  Resolver(*this).resolve(*config);

  // Check JmpAPI version
  const Version minVer("1.1.0");
  Version version(config->getString("jmpapi", "0.0.0"));
  if (version < minVer) THROW("API version must be at least " << minVer);

  // Create Spec
  spec = config->createDict();
  spec->insert("openapi", "3.1.0");
  if (config->hasDict("info")) spec->insert("info", config->get("info"));
  spec->insert("tags",  config->createList());
  spec->insert("paths", config->createDict());

  // Always parse args
  addHandler(new ArgsParser);

  // Get list of APIs
  JSON::ValuePtr apis;
  if (config->hasDict("apis")) apis = config->get("apis");
  else {
    apis = new JSON::Dict;
    apis->insert("", config->get("api"));
  }

  // Pass 1: Register queries before timeseries and endpoints
  for (auto e: apis->entries()) {
    auto api = e.value();
    category = e.key();

    if (api->hasDict("queries"))
      for (auto e2: api->get("queries")->entries())
        addQuery(e2.key(), e2.value());
  }

  // Pass 2: Register timeseries before endpoints
  for (auto e: apis->entries()) {
    auto api = e.value();
    category = e.key();

    if (api->hasDict("timeseries"))
      for (auto e2: api->get("timeseries")->entries())
        addTimeseries(e2.key(), e2.value());
  }

  // Pass 3: Register endpoints and create spec
  for (auto e: apis->entries()) {
    auto api = e.value();
    category = e.key();

    addTagSpec(e.key(), api);

    if (api->hasDict("endpoints"))
      addHandler(createAPIHandler(new Context(*this, api->get("endpoints"))));
  }

  category.clear();
}


void cb::API::API::bind(const string &key, const RequestHandlerPtr &handler) {
  if (!callbacks.insert(decltype(callbacks)::value_type(key, handler)).second)
    THROW("API binding for '" << key << "' already exists");
}


string cb::API::API::resolve(const string &name) const {
  if (category.empty()) return name;
  return name.find('.') == string::npos ? (category + "." + name) : name;
}


void cb::API::API::addQuery(
  const string &name, const SmartPointer<QueryDef> &def) {
  string _name = resolve(name);
  auto result = queries.insert(decltype(queries)::value_type(_name, def));
  if (!result.second) THROW("Query '" << _name << "' already exists");
  LOG_INFO(3, "Adding query " << _name);
}


SmartPointer<QueryDef> cb::API::API::addQuery(
  const string &name, const JSON::ValuePtr &config) {
  auto def = SmartPtr(new QueryDef(*this, config));
  addQuery(name, def);
  return def;
}


const SmartPointer<QueryDef> &cb::API::API::getQuery(const string &name) const {
  string _name = resolve(name);
  auto it = queries.find(_name);
  if (it == queries.end())
    THROWX("Query '" << _name << "' not found", HTTP_NOT_FOUND);
  return it->second;
}


void cb::API::API::addTimeseries(
  const string &name, const SmartPointer<TimeseriesDef> &ts) {
  if (timeseriesDB.isNull())
    THROW("Cannot configure timeseries with out timeseries DB");
  string _name = resolve(name);
  auto result = timeseries.insert(decltype(timeseries)::value_type(_name, ts));
  if (!result.second) THROW("Timeseries '" << _name << "' already exists");
  LOG_INFO(3, "Adding timeseries " << _name);
  ts->load();
}


SmartPointer<TimeseriesDef> cb::API::API::addTimeseries(
  const string &name, const JSON::ValuePtr &config) {
  auto ts = SmartPtr(new TimeseriesDef(*this, name, config));
  addTimeseries(name, ts);
  return ts;
}


const SmartPointer<TimeseriesDef> &cb::API::API::getTimeseries(
  const string &name) const {
  string _name = resolve(name);
  auto it = timeseries.find(_name);
  if (it == timeseries.end())
    THROWX("Timeseries '" << _name << "' not found", HTTP_NOT_FOUND);
  return it->second;
}


string cb::API::API::getEndpointType(const JSON::ValuePtr &config) const {
  if (config->has("handlers")) THROW("Nested handlers not allowed");

  string type = config->getString("handler", "");
  if (!type.empty()) return type;

  if (config->has("bind"))       return "bind";
  if (config->has("timeseries")) return "timeseries";
  if (config->has("sql"))        return "query";
  if (config->has("query"))      return "query";
  if (config->has("path"))       return "file";
  if (config->has("resource"))   return "resource";

  return "pass";
}


JSON::ValuePtr cb::API::API::getEndpointTypes(
  const JSON::ValuePtr &config) const {
  if (config->has("handlers")) {
    if (config->has("handler"))
      THROW("Cannot define both 'handler' and 'handlers'");

    auto types = config->createList();

    auto &handlers = config->getList("handlers");
    for (auto &handler: handlers)
      types->append(getEndpointType(handler));

    return types;
  }

  return config->create(getEndpointType(config));
}


HTTP::RequestHandlerPtr cb::API::API::createEndpointHandler(
  const JSON::ValuePtr &types, const JSON::ValuePtr &config) {
  // Multiple handlers
  if (types->isList()) {
    auto group     = SmartPtr(new HandlerGroup);
    auto &handlers = config->getList("handlers");

    for (unsigned i = 0; i < types->size(); i++) {
      auto config = handlers.get(i);
      group->addHandler(createEndpointHandler(types->get(i), config));
    }

    return group;
  }

  // A single handler
  string type = types->asString();

  if (type == "bind") {
    auto key = config->getString("bind", "<default>");
    auto it  = callbacks.find(key);
    if (it == callbacks.end()) THROW("Bind callback '" << key << "' not found");
    return it->second;
  }

  if (type == "pass")      return new PassHandler;
  if (type == "cors")      return new CORSHandler(config);
  if (type == "status")    return new StatusHandler(config);
  if (type == "redirect")  return new RedirectHandler(config);
  if (type == "spec")      return new SpecHandler(*this, config);
  if (type == "file")      return new HTTP::FileHandler(config);
  if (type == "websocket") return new WebsocketHandler(*this, config);
  if (type == "resource")
    return new HTTP::ResourceHandler(config->getString("resource"));

  if (client.isSet() && oauth2Providers.isSet() && sessionManager.isSet()) {
    if (config->hasString("sql") && connector.isNull())
      THROW("Cannot have 'sql' in API without a DB connector");

    if (type == "login")   return new LoginHandler  (*this, config);
    if (type == "logout")  return new LogoutHandler (*this, config);
    if (type == "session") return new SessionHandler(*this, config);
  }

  if (connector.isSet()) {
    if (type == "query")      return new QueryHandler     (*this, config);
    if (type == "timeseries") return new TimeseriesHandler(*this, config);
  }

  THROW("Unsupported handler '" << type << "'");
}


HTTP::RequestHandlerPtr cb::API::API::createMethodsHandler(
  const string &methods, const CtxPtr &ctx) {
  auto group   = SmartPtr(new HTTP::HandlerGroup);
  auto &config = ctx->getConfig();
  auto types   = getEndpointTypes(config);

  LOG_INFO(3, "Adding endpoint " << methods << " " << ctx->getPattern()
    << " (" << types->toString(0, true) << ")");

  addToSpec(methods, ctx);

  // Validation (must be first)
  ctx->addValidation(*group);

  // Headers
  if (config->has("headers"))
    group->addHandler(new HeadersHandler(config->get("headers")));

  // Handler
  auto handler = createEndpointHandler(types, config);

  // Arg filter
  if (config->has("arg-filter")) {
    if (procPool.isNull())
      THROW("API cannot have 'arg-filter' without Event::SubprocessPool");

    auto filter = config->get("arg-filter");
    handler = new ArgFilterHandler(*this, filter, handler);
  }

  group->addHandler(handler);

  return group;
}


HTTP::RequestHandlerPtr cb::API::API::createAPIHandler(const CtxPtr &ctx) {
  auto children = SmartPtr(new HTTP::HandlerGroup);
  auto methods  = SmartPtr(new HTTP::HandlerGroup);
  auto &api     = ctx->getConfig();
  auto &pattern = ctx->getPattern();

  // Children
  for (auto e: api->entries()) {
    auto &key   = e.key();
    auto config = e.value();

    if (config->isString()) {
      string bind = config->getString();
      config = config->createDict();
      config->insert("bind", bind);
    }

    // Child
    if (!key.empty() && key[0] == '/') {
      children->addHandler(createAPIHandler(ctx->createChild(config, key)));
      continue;
    }

    // Methods
    unsigned methodTypes = parseMethods(key);
    if (methodTypes) {
      auto handler = createMethodsHandler(key, ctx->createChild(config, ""));
      methods->addHandler(new HTTP::MethodMatcher(methodTypes, handler));
    }
  }

  auto group = SmartPtr(new HTTP::HandlerGroup);

  if (!methods->isEmpty()) {
    if (pattern.empty()) group->addHandler(methods);
    else group->addHandler(new HTTP::URLPatternMatcher(pattern, methods));
  }

  if (!children->isEmpty()) {
    if (pattern.empty()) group->addHandler(children);
    else group->addHandler(
      new HTTP::URLPatternMatcher(pattern + ".+", children));
  }

  return group;
}


void cb::API::API::addTagSpec(const string &tag, const JSON::ValuePtr &config) {
  hideCategory = config->getBoolean("hide", false);
  if (hideCategory) return;

  auto tags    = spec->get("tags");
  auto tagSpec = config->createDict();
  tags->append(tagSpec);

  tagSpec->insert("name", tag);
  if (config->hasString("help"))
    tagSpec->insert("description", config->get("help"));
}


void cb::API::API::addToSpec(const string &methods, const CtxPtr &ctx) {
  if (hideCategory) return;

  auto config = ctx->getConfig();
  if (config->getBoolean("hide", false)) return;

  auto paths  = spec->get("paths");
  auto path   = ctx->getPattern();

  if (!paths->hasDict(path)) paths->insert(path, config->createDict());
  auto pathSpec = paths->get(path);

  vector<string> methodNames;
  String::tokenize(methods, methodNames, "|");
  for (auto &method: methodNames) {
    auto methodSpec = config->createDict();
    pathSpec->insert(String::toLower(method), methodSpec);

    // Add tag from category
    if (!category.empty()) {
      auto tags = config->createList();
      methodSpec->insert("tags", tags);
      tags->append(category);
    }

    // Add description from help
    if (config->hasString("help"))
      methodSpec->insert("description", config->get("help"));

    // Add parameter spec from path and args handlers
    set<string> foundArgs;
    Regex re(HTTP::URLPatternMatcher::toRE2Pattern(path), false);
    auto &urlArgs = re.getGroupNameMap();
    JSON::ValuePtr paramSpec = config->createList();
    methodSpec->insert("parameters", paramSpec);

    auto args = ctx->getArgsHandler();
    if (args.isSet()) {
      args->appendSpecs(*paramSpec);

      // Determine arg "in" type
      for (auto &spec: *paramSpec) {
        string name = spec->getString("name");
        bool isURLArg = urlArgs.find(name) != urlArgs.end();
        spec->insert("in", isURLArg ? "path" : "query");
        foundArgs.insert(name);
      }
    }

    // Add missing URL args to spec
    for (auto &arg: urlArgs)
      if (foundArgs.find(arg.first) == foundArgs.end()) {
        auto argSpec = config->createDict();
        argSpec->insert("name", arg.first);
        argSpec->insert("in", "path");
        argSpec->insertBoolean("required", true);
        auto schema = config->createDict();
        argSpec->insert("schema", schema);
        schema->insert("type", "string");
        paramSpec->append(argSpec);
      }

    // TODO Add security spec from access handlers
    // TODO add response schemas
  }
}
