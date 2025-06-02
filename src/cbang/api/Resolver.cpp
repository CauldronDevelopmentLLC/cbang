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

#include "Resolver.h"
#include "API.h"

#include <cbang/log/Logger.h>
#include <cbang/json/Null.h>
#include <cbang/json/String.h>
#include <cbang/config/Options.h>
#include <cbang/http/Request.h>
#include <cbang/db/maria/DB.h>

#include <set>

using namespace std;
using namespace cb;
using namespace cb::API;


Resolver::Resolver(API &api) {
  vars.insert("args", new JSON::Dict);
  vars.insert("options", api.getOptions());
}


Resolver::Resolver(API &api, const HTTP::Request &req) : Resolver(api) {
  set("args", req.getArgs());
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


string Resolver::resolve(const string &s, bool sql) const {
  auto cb = [&] (const string &id, const string &spec) -> string {
    auto value = select(id);

    if (value.isSet()) {
      if ((sql && spec.empty() && value->isString()) || spec == "S")
        return MariaDB::DB::format(value->asString());

      return value->formatAs(spec);
    }

    if (sql) return "NULL";

    return "{" + id + (spec.empty() ? string() : (":" + spec)) + "}";
  };

  return String(s).format(cb);
}


void Resolver::resolve(JSON::Value &value, bool sql) const {
  if (value.isString()) {
    auto sPtr = dynamic_cast<JSON::String *>(&value);
    if (!sPtr) THROW("Expected JSON::String");

    string &s = sPtr->getValue();
    if (s.find('{') != string::npos) s = resolve(s, sql);

  } else if (value.isList() || value.isDict())
    for (auto e: value.entries())
      resolve(*e.value(), sql);
}
