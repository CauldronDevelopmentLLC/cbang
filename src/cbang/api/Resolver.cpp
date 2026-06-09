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
#include <cbang/db/maria/DB.h>

#include <set>
#include <vector>

using namespace std;
using namespace cb;
using namespace cb::API;


Resolver::Resolver(API &api) {
  vars.insert("options", api.getOptions());
}


Resolver::Resolver(API &api, const HTTP::Request &req) : Resolver(api) {
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

  // Return ``null`` if not found
  if (path[0] == '~') {
    auto result = select(path.substr(1));
    if (result.isNull()) return JSON::Null::instancePtr();
    return result;
  }

  auto result = vars.select(path, 0);
  if (result.isSet()) return result;
  return parent.isSet() ? parent->select(path) : 0;
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


string Resolver::resolve(const string &s, bool sql) const {
  return resolve(s, sql, 0);
}


string Resolver::resolveSQL(const string &s, vector<string> &params) const {
  return resolve(s, true, &params);
}


string Resolver::resolve(
  const string &s, bool sql, vector<string> *params) const {
  auto cb = [&] (const string &id, const string &spec) -> string {
    auto value = select(id);

    if (sql) {
      auto *blob = dynamic_cast<Blob *>(value.get());
      if (blob) {
        if (!params) THROW("Binary value '" << id << "' not supported here");
        params->push_back(blob->getData());
        return "?";
      }

      if (value.isNull() || value->isNull() || value->isUndefined())
        return "NULL";

      if (spec == "S" ||
          (spec.empty() && !value->isNumber() && !value->isBoolean()))
        return MariaDB::DB::format(value->asString());
    }

    if (value.isSet()) return value->formatAs(spec);

    return "{" + id + (spec.empty() ? string() : (":" + spec)) + "}";
  };

  return String(s).format(cb);
}


JSON::ValuePtr Resolver::resolveValue(
  const JSON::ValuePtr &value, bool sql) const {
  if (value->isList() || value->isDict()) {resolve(*value, sql); return value;}

  if (value->isString()) {
    const string &s = value->getString();

    // A lone reference with no format spec resolves to the native value
    if (!sql && 2 <= s.length() && s.front() == '{' && s.back() == '}') {
      string id = s.substr(1, s.length() - 2);
      if (id.find_first_of("{}:") == string::npos) {
        auto v = select(id);
        return v.isSet() ? v->copy(true) : value; // Missing: leave literal
      }
    }

    if (s.find('{') != string::npos) return new JSON::String(resolve(s, sql));
  }

  return value;
}


void Resolver::resolve(JSON::Value &value, bool sql) const {
  if (value.isList())
    for (unsigned i = 0; i < value.size(); i++)
      value.set(i, resolveValue(value.get(i), sql));

  else if (value.isDict()) {
    // Collect keys first; resolveValue may replace entries in place
    std::vector<std::string> keys;
    for (auto e: value.entries()) keys.push_back(e.key());
    for (auto &key: keys) value.insert(key, resolveValue(value.get(key), sql));

  } else if (value.isString()) {
    // Top-level string: resolve in place (cannot retype without a parent)
    auto sPtr = dynamic_cast<JSON::String *>(&value);
    if (sPtr && sPtr->getValue().find('{') != string::npos)
      sPtr->getValue() = resolve(sPtr->getValue(), sql);
  }
}
