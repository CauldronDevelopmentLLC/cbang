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

#include "Resolver.h"
#include "API.h"
#include "Blob.h"

#include <cbang/log/Logger.h>
#include <cbang/json/Null.h>
#include <cbang/json/String.h>
#include <cbang/config/Options.h>
#include <cbang/http/Request.h>

#include <set>
#include <vector>

using namespace std;
using namespace cb;
using namespace cb::API;


namespace {
  // The {request.*} root.  Values materialize on first reference, so an
  // unreferenced value costs nothing.  Holds the Request, like Context, so
  // the request must outlive resolution.
  //
  // Note, behind a reverse proxy ``host`` and ``ip`` are what the proxy
  // sends; e.g. nginx needs ``proxy_set_header Host $host;`` to pass the
  // original host and ``ip`` is the proxy's address.
  class HeaderDict : public JSON::Dict {
    const HTTP::Request &req;

  public:
    HeaderDict(const HTTP::Request &req) : req(req) {}

    Iterator find(const string &key) const override {
      auto it = JSON::Dict::find(key);
      if (!it && req.inHas(key)) {
        const_cast<HeaderDict *>(this)->insert(key, req.inFind(key));
        it = JSON::Dict::find(key);
      }
      return it;
    }
  };


  class RequestDict : public JSON::Dict {
    const HTTP::Request &req;

  public:
    RequestDict(const HTTP::Request &req) : req(req) {}

    Iterator find(const string &key) const override {
      auto it = JSON::Dict::find(key);

      if (!it) {
        auto *self = const_cast<RequestDict *>(this);

        if (key == "method")
          self->insert(key, req.getMethod().toString());
        else if (key == "host")    self->insert(key, req.inFind("Host"));
        else if (key == "path")    self->insert(key, req.getURI().getPath());
        else if (key == "ip")
          self->insert(key, req.getClientAddr().toString(false));
        else if (key == "headers") self->insert(key, new HeaderDict(req));
        else return it;

        it = JSON::Dict::find(key);
      }

      return it;
    }
  };
}


Resolver::Resolver(API &api) {
  vars.insert("options", api.getOptions());
}


Resolver::Resolver(API &api, const HTTP::Request &req) : Resolver(api) {
  set("request", new RequestDict(req));
  setSession(req.getSession());
}


void Resolver::set(const string &key, const JSON::ValuePtr &values) {
  vars.insert(key, values);
}


void Resolver::setSession(const SmartPointer<HTTP::Session> &session) {
  if (session.isSet()) {
    set("session", session);
    set("group",   session->select("group"));
  }
}


JSON::ValuePtr Resolver::select(const string &path) const {
  if (path.empty()) return 0;

  auto result = vars.select(path, 0);
  if (result.isSet()) return result;
  return parent.isSet() ? parent->select(path) : 0;
}


// A ``~`` marks the ref optional: missing resolves to null rather than an
// error, except in a partial resolve which leaves it for a later resolve.
JSON::ValuePtr Resolver::selectRef(const string &id, bool partial) const {
  bool opt = !id.empty() && id[0] == '~';
  auto value = select(opt ? id.substr(1) : id);
  if (value.isNull() && opt && !partial) return JSON::Null::instancePtr();
  return value;
}


string Resolver::selectString(const string &path) const {
  auto value = select(path);
  if (value.isNull()) THROW("String value '" << path << "' not found");
  return value->asString();
}


string Resolver::selectString(
  const string &path, const string &defaultValue) const {
  auto value = select(path);
  return value.isNull() ? defaultValue : value->asString();
}


uint64_t Resolver::selectU64(const string &path, uint64_t defaultValue) const {
  auto value = select(path);
  return value.isNull() ? defaultValue : value->getU64();
}


uint64_t Resolver::selectTime(const string &path, uint64_t defaultValue) const {
  auto value = select(path);
  return value.isNull() ? defaultValue : Time::parse(value->asString());
}


string Resolver::resolve(const string &s, bool partial) const {
  return resolve(s, partial, 0);
}


string Resolver::resolveSQL(
  const string &s, vector<JSON::ValuePtr> &params) const {
  return resolve(s, false, &params);
}


string Resolver::resolve(
  const string &s, bool partial, vector<JSON::ValuePtr> *params) const {
  auto cb = [&] (const string &id, const string &spec) -> string {
    auto value = selectRef(id, partial);

    if (value.isNull()) {
      if (partial) // Leave for a later resolve with more vars
        return "{" + id + (spec.empty() ? string() : (":" + spec)) + "}";
      THROW("Variable '" << id << "' not found; use {~" << id
            << "} to resolve null when missing");
    }

    if (params) { // SQL: every ref is bound as a statement parameter
      auto *blob = dynamic_cast<Blob *>(value.get());
      params->push_back(
        blob ? new JSON::String(blob->getData()) :
        spec.empty() || value->isNull() ? value :
        new JSON::String(value->formatAs(spec)));
      return "?";
    }

    return value->formatAs(spec);
  };

  return String(s).format(cb);
}


JSON::ValuePtr Resolver::resolveValue(
  const JSON::ValuePtr &value, bool partial) const {
  if (value->isList() || value->isDict()) {
    resolve(*value, partial);
    return value;
  }

  if (value->isString()) {
    const string &s = value->getString();

    // A lone reference with no format spec resolves to the native value
    if (2 <= s.length() && s.front() == '{' && s.back() == '}') {
      string id = s.substr(1, s.length() - 2);
      if (id.find_first_of("{}:") == string::npos) {
        auto v = selectRef(id, partial);
        if (v.isSet()) return v->copy(true);
        if (partial) return value; // Leave for a later resolve
        THROW("Variable '" << id << "' not found; use {~" << id
              << "} to resolve null when missing");
      }
    }

    if (s.find('{') != string::npos)
      return new JSON::String(resolve(s, partial));
  }

  return value;
}


void Resolver::resolve(JSON::Value &value, bool partial) const {
  if (value.isList())
    for (unsigned i = 0; i < value.size(); i++)
      value.set(i, resolveValue(value.get(i), partial));

  else if (value.isDict()) {
    // Collect keys first; resolveValue may replace entries in place
    std::vector<std::string> keys;
    for (auto e: value.entries()) keys.push_back(e.key());
    for (auto &key: keys)
      value.insert(key, resolveValue(value.get(key), partial));

  } else if (value.isString()) {
    // Top-level string: resolve in place (cannot retype without a parent)
    auto sPtr = dynamic_cast<JSON::String *>(&value);
    if (sPtr && sPtr->getValue().find('{') != string::npos)
      sPtr->getValue() = resolve(sPtr->getValue(), partial);
  }
}
