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

#include "Resolver.h"
#include "API.h"

#include <cbang/log/Logger.h>
#include <cbang/json/Null.h>
#include <cbang/json/String.h>
#include <cbang/config/Options.h>

#include <set>

using namespace std;
using namespace cb;
using namespace cb::API;


Resolver::Resolver(
  API &api, const JSON::ValuePtr &ctx, const ResolverPtr &parent) :
  api(api), req(parent->req), ctx(ctx), parent(parent) {}


Resolver::Resolver(API &api, const RequestPtr &req) : api(api), req(req) {}


ResolverPtr Resolver::makeChild(const JSON::ValuePtr &ctx) {
  return new Resolver(api, ctx, this);
}


JSON::ValuePtr Resolver::select(const string &name) const {
  if (name.empty()) return 0;
  if (name == ".") return ctx;

  // Return ``null`` if not found
  if (name[0] == '~') {
    auto result = select(name.substr(1));
    if (result.isNull()) return JSON::Null::instancePtr();
    return result;
  }

  if (name == "..") return parent->getContext();
  if (String::startsWith(name, "../")) {
    if (parent.isSet()) return parent->select(name.substr(3));
    return 0;
  }

  if (req.isSet()) {
    if (name == "args") return req->getArgs();
    if (String::startsWith(name, "args."))
      return req->getArgs()->select(name.substr(5), 0);

    auto &session = req->getSession();
    if (session.isSet()) {
      if (name == "session") return session;
      if (String::startsWith(name, "session."))
        return session->select(name.substr(8), 0);

      if (name == "group") return session->get("group");
      if (String::startsWith(name, "group."))
        return session->get("group")->select(name.substr(6), 0);
    }
  }

  if (String::startsWith(name, "options."))
    return new JSON::String(api.getOptions()[name.substr(8)]);

  if (ctx.isSet()) {
    if (String::startsWith(name, "./")) return ctx->select(name.substr(2), 0);
    return ctx->select(name, 0);
  }

  return 0;
}


string Resolver::format(const string &s,
                        String::format_cb_t default_cb) const {
  String::format_cb_t cb =
    [&] (char type, int index, const string &name, bool &matched) {
      // Do not allow recursive refs, user input could contain var refs
      auto value = select(name);
      if (value.isSet()) return value->format(type);

      // TODO Is there something wrong with matching bools?: %(test)b

      if (default_cb) return default_cb(type, index, name, matched);

      matched = false;
      return String::printf("%%(%s)%c", name.c_str(), type);
    };

  return String(s).format(cb);
}


string Resolver::format(const string &s, const string &defaultValue) const {
  auto cb = [&] (char, int, const string &, bool &) {return defaultValue;};
  return format(s, cb);
}


void Resolver::resolve(JSON::Value &value) const {
  auto cb =
    [&] (JSON::Value &value, JSON::Value *parent, unsigned index) {
      if (!value.isString()) return;

      string s = value.getString();
      if (s.find('%') == string::npos) return;

      value.cast<JSON::String>().getValue() = format(s);
    };

  value.visit(cb);
}
