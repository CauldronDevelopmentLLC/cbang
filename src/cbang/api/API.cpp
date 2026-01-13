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

#include "API.h"

#include "Resolver.h"
#include "QueryDef.h"
#include "HandlerGroup.h"

#include <cbang/api/handler/MethodHandler.h>
#include <cbang/api/handler/URLHandler.h>
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
#include <cbang/api/handler/HTTPHandler.h>
#include <cbang/api/arg/ArgDict.h>

#include <cbang/http/Request.h>
#include <cbang/http/URLPatternMatcher.h>
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

  // Get list of APIs
  JSON::ValuePtr apis;
  if (config->hasDict("apis")) apis = config->get("apis");
  else {
    apis = new JSON::Dict;
    apis->insert("", config->get("api"));
  }

  // Pass 1: Register args
  for (auto e: apis->entries()) {
    auto api = e.value();
    category = e.key();

    if (api->hasDict("args"))
      for (auto e2: api->get("args")->entries())
        addArgs(e2.key(), e2.value());
  }

  // Pass 2: Register queries
  for (auto e: apis->entries()) {
    auto api = e.value();
    category = e.key();

    if (api->hasDict("queries"))
      for (auto e2: api->get("queries")->entries()) {
        HandlerPtr handler = new QueryHandler(*this, e2.value());
        handler = wrapEndpoint(handler, new Config(*this, e2.value()));
        addQuery(e2.key(), handler);
      }
  }

  // Pass 3: Register timeseries
  if (timeseriesDB.isSet())
    for (auto e: apis->entries()) {
      auto api = e.value();
      category = e.key();

      if (api->hasDict("timeseries"))
        for (auto e2: api->get("timeseries")->entries()) {
          string name = category + "." + e2.key();
          HandlerPtr handler = new TimeseriesHandler(*this, name, e2.value());
          handler = wrapEndpoint(handler, new Config(*this, e2.value()));
          addTimeseries(e2.key(), handler);
        }
    }

  // Pass 4: Register endpoints and create spec
  for (auto e: apis->entries()) {
    auto api = e.value();
    category = e.key();

    addTagSpec(e.key(), api);

    if (api->hasDict("endpoints"))
      add(createAPIHandler(new Config(*this, api->get("endpoints"))));
  }

  category.clear();
}


void cb::API::API::bind(const string &key, const RequestHandlerPtr &handler) {
  if (!callbacks.insert(decltype(callbacks)::value_type(key, handler)).second)
    THROW("API binding for '" << key << "' already exists");
}


string cb::API::API::resolve(const string &category, const string &name) {
  if (category.empty()) return name;
  return name.find('.') == string::npos ? (category + "." + name) : name;
}


string cb::API::API::resolve(const string &name) const {
  return resolve(category, name);
}


void cb::API::API::addArgs(const string &name, const JSON::ValuePtr &_args) {
  if (!_args->isDict())
    THROW("Args def is not a dictionary: " << _args->toString());

  string _name = resolve(name);
  auto result = args.insert(decltype(args)::value_type(_name, _args));
  if (!result.second) THROW("Args '" << _name << "' already exists");

  LOG_INFO(3, "Adding args " << _name);
}


const JSON::ValuePtr &cb::API::API::getArgs(const string &name) const {
  string _name = resolve(name);
  auto it = args.find(_name);

  if (it == args.end())
    THROWX("Args '" << _name << "' not found", HTTP_NOT_FOUND);

  return it->second;
}


void cb::API::API::addQuery(const string &name, const HandlerPtr &handler) {
  string _name = resolve(name);
  auto result = queries.insert(decltype(queries)::value_type(_name, handler));
  if (!result.second) THROW("Query '" << _name << "' already exists");

  LOG_INFO(3, "Adding query " << _name);
}


cb::API::HandlerPtr cb::API::API::getQuery(
  const string &category, const string &name) const {

  string _name = resolve(category, name);
  auto it = queries.find(_name);

  if (it == queries.end())
    THROWX("Query '" << _name << "' not found", HTTP_NOT_FOUND);

  return it->second;
}


cb::API::HandlerPtr cb::API::API::getQuery(const string &name) const {
  return getQuery(category, name);
}


cb::API::HandlerPtr cb::API::API::getQuery(const JSON::ValuePtr &config) {
  if (config->hasString("query")) {
    if (config->has("sql")) THROW("Cannot define both 'query' and 'sql'");
    return getQuery(config->getString("query"));
  }

  return new QueryHandler(*this, config);
}


void cb::API::API::addTimeseries(
  const string &name, const HandlerPtr &handler) {

  if (timeseriesDB.isNull())
    THROW("Cannot configure timeseries with out timeseries DB");

  string _name = resolve(name);
  auto result =
    timeseries.insert(decltype(timeseries)::value_type(_name, handler));
  if (!result.second) THROW("Timeseries '" << _name << "' already exists");
  LOG_INFO(3, "Adding timeseries " << _name);
}


cb::API::HandlerPtr cb::API::API::getTimeseries(
  const string &category, const string &name) const {
  string _name = resolve(category, name);
  auto it = timeseries.find(_name);

  if (it == timeseries.end())
    THROWX("Timeseries '" << _name << "' not found", HTTP_NOT_FOUND);

  return it->second;
}


cb::API::HandlerPtr cb::API::API::getTimeseries(const string &name) const {
  return getTimeseries(category, name);
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


cb::API::HandlerPtr cb::API::API::createEndpointHandler(
  const JSON::ValuePtr &types, const CfgPtr &cfg) {
  auto config = cfg->getConfig();

  // Multiple handlers
  if (types->isList()) {
    auto group     = SmartPtr(new HandlerGroup);
    auto &handlers = config->getList("handlers");

    for (unsigned i = 0; i < types->size(); i++) {
      auto childCfg = cfg->createChild(handlers.get(i));
      group->add(createEndpointHandler(types->get(i), childCfg));
    }

    return group;
  }

  // A single handler
  string type = types->asString();

  if (type == "bind") {
    auto key = config->getString("bind", "<default>");
    auto it  = callbacks.find(key);
    if (it == callbacks.end()) THROW("Bind callback '" << key << "' not found");
    return new HTTPHandler(it->second);
  }

  if (type == "pass")      return new PassHandler;
  if (type == "cors")      return new CORSHandler(config);
  if (type == "status")    return new StatusHandler(config);
  if (type == "redirect")  return new RedirectHandler(config);
  if (type == "spec")      return new SpecHandler(*this, config);
  if (type == "websocket") return new WebsocketHandler(*this, config);
  if (type == "file")
    return new HTTPHandler(new HTTP::FileHandler(config));
  if (type == "resource")
    return new HTTPHandler(
      new HTTP::ResourceHandler(config->getString("resource")));

  if (client.isSet() && oauth2Providers.isSet() && sessionManager.isSet()) {
    if (config->hasString("sql") && connector.isNull())
      THROW("Cannot have 'sql' in API without a DB connector");

    if (type == "login")   return new LoginHandler  (*this, config);
    if (type == "logout")  return new LogoutHandler (*this, config);
    if (type == "session") return new SessionHandler(*this, config);
  }

  if (connector.isSet()) {
    if (type == "query") {
      if (config->has("query")) {
        if (config->has("sql")) THROW("Cannot define both 'query' and 'sql'");
        return getQuery(config->getString("query"));
      }

      return new QueryHandler(*this, config);
    }

    if (type == "timeseries" && timeseriesDB.isSet()) {
      if (config->has("timeseries")) {
        if (config->has("query") || config->has("sql"))
          THROW("Cannot define both 'timeseries' and 'query' or 'sql'");

        return getTimeseries(config->getString("timeseries"));
      }

      string name = category + "." + cfg->getPattern();
      return new TimeseriesHandler(*this, name, config);
    }
  }

  THROW("Unsupported handler '" << type << "'");
}


cb::API::HandlerPtr cb::API::API::wrapEndpoint(
  const HandlerPtr &handler, const CfgPtr &cfg) {
  auto group = SmartPtr(new HandlerGroup);

  // Headers
  if (config->has("headers"))
    group->add(new HeadersHandler(config->get("headers")));

  // Arg filter
  if (config->has("arg-filter")) {
    if (procPool.isNull())
      THROW("API cannot have 'arg-filter' without Event::SubprocessPool");

    auto filter = config->get("arg-filter");
    group->add(new ArgFilterHandler(*this, filter, handler));

  } else group->add(handler);

  // Validation
  return cfg->addValidation(group);
}


cb::API::HandlerPtr cb::API::API::createMethodsHandler(
  const string &methods, const CfgPtr &cfg) {
  auto config = cfg->getConfig();
  auto types  = getEndpointTypes(config);

  LOG_INFO(3, "Adding endpoint " << methods << " " << cfg->getPattern()
    << " (" << types->toString(0, true) << ")");

  addToSpec(methods, cfg);

  return wrapEndpoint(createEndpointHandler(types, cfg), cfg);
}


cb::API::HandlerPtr cb::API::API::createAPIHandler(const CfgPtr &cfg) {
  auto children = SmartPtr(new HandlerGroup);
  auto methods  = SmartPtr(new HandlerGroup);
  auto api      = cfg->getConfig();
  auto pattern  = cfg->getPattern();

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
      children->add(createAPIHandler(cfg->createChild(config, key)));
      continue;
    }

    // Methods
    unsigned methodTypes = parseMethods(key);
    if (methodTypes) {
      auto handler = createMethodsHandler(key, cfg->createChild(config));
      methods->add(new MethodHandler(methodTypes, handler));
    }
  }

  auto group = SmartPtr(new HandlerGroup);

  if (!methods->isEmpty()) {
    if (pattern.empty()) group->add(methods);
    else group->add(new URLHandler(pattern, methods));
  }

  if (!children->isEmpty()) {
    if (pattern.empty()) group->add(children);
    else group->add(new URLHandler(pattern + ".+", children));
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


void cb::API::API::addToSpec(const string &methods, const CfgPtr &cfg) {
  if (hideCategory) return;

  auto config = cfg->getConfig();
  if (config->getBoolean("hide", false)) return;

  auto paths = spec->get("paths");
  auto path  = cfg->getPattern();

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

    auto args = cfg->getArgs();
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


bool cb::API::API::operator()(HTTP::Request &req) {
  auto ctx = SmartPtr(new Context(*this, req));
  ctx->setArgs(req.parseArgs());
  return operator()(ctx);
}
