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

#include "BodyHandler.h"

#include <cbang/api/Resolver.h>
#include <cbang/Exception.h>
#include <cbang/String.h>

using namespace std;
using namespace cb;
using namespace cb::API;


namespace {
  // A byte count, allowing a K/M/G suffix with optional B, e.g. ``5MB``
  uint64_t parseSize(const JSON::Value &value) {
    if (value.isNumber()) return value.getU64();

    string s = String::toUpper(String::trim(value.asString()));
    unsigned shift = 0;

    if (!s.empty() && s.back() == 'B') s.pop_back();
    if (!s.empty())
      switch (s.back()) {
      case 'K': shift = 10; s.pop_back(); break;
      case 'M': shift = 20; s.pop_back(); break;
      case 'G': shift = 30; s.pop_back(); break;
      }

    return String::parseU64(String::trim(s), true) << shift;
  }


  // ``allowed`` is a media type, a ``*`` glob (e.g. ``image/*``), or a list
  bool typeMatches(const string &type, const JSON::Value &allowed) {
    if (allowed.isList()) {
      for (auto &t: allowed)
        if (typeMatches(type, *t)) return true;
      return false;
    }

    string pat = String::toLower(allowed.asString());
    if (String::endsWith(pat, "/*"))
      return String::startsWith(type, pat.substr(0, pat.length() - 1));
    return type == pat;
  }
}


void BodyHandler::operator()(const CtxPtr &ctx, const Cont &next) {
  if (body.isSet()) validate(ctx, "body", "Request body", *body);

  if (files.isSet())
    for (auto e: files->entries())
      validate(ctx, "files." + e.key(), "File '" + e.key() + "'", *e.value());

  (*child)(ctx, next);
}


void BodyHandler::validate(const CtxPtr &ctx, const string &ref,
                           const string &what, const JSON::Value &spec) const {
  auto value = ctx->getResolver()->select(ref);

  if (value.isNull()) {
    if (spec.getBoolean("required", false))
      THROWX(what << " is required", HTTP_BAD_REQUEST);
    return;
  }

  if (spec.has("max-size") &&
      parseSize(*spec.get("max-size")) < value->getU64("size", 0))
    THROWX(what << " exceeds maximum size "
           << spec.get("max-size")->asString(),
           HTTP_REQUEST_ENTITY_TOO_LARGE);

  if (spec.has("type")) {
    string type = value->getString("type", "");
    type = String::toLower(String::trim(type.substr(0, type.find(';'))));

    if (!typeMatches(type, *spec.get("type")))
      THROWX(what << " type '" << type << "' not allowed",
             HTTP_UNSUPPORTED_MEDIA_TYPE);
  }
}
