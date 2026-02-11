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

#include "CORSHandler.h"

#include <cbang/String.h>
#include <cbang/http/Request.h>
#include <cbang/http/Method.h>

using namespace std;
using namespace cb;
using namespace cb::API;


CORSHandler::CORSHandler(const JSON::ValuePtr &config) {
  // Explicit origins
  if (config->has("origins"))
    for (auto &v: *config->get("origins"))
      origins.insert(String::toLower(v->getString()));

  // Origin regex patterns
  if (config->has("patterns"))
    for (auto &v: *config->get("patterns"))
      patterns.push_back(v->getString());

  // Add defaults
  auto hdrs = config->get("headers", new JSON::Dict);

  const char *name = "Access-Control-Allow-Origin";
  if (config->has("origin") && !hdrs->has(name))
    add(name, config->getAsString("origin"));

  name = "Access-Control-Allow-Methods";
  if (!hdrs->has(name))
    add(name, config->getString("methods", "POST,PUT,GET,OPTIONS,DELETE"));

  name = "Access-Control-Allow-Headers";
  if (!hdrs->has(name))
    add(name, "DNT,User-Agent,X-Requested-With,If-Modified-Since,"
        "Cache-Control,Content-Type,Range,Set-Cookie,Authorization");

  name = "Access-Control-Allow-Credentials";
  if (config->getBoolean("credentials", false) && !hdrs->has(name))
    add(name, "true");

  name = "Access-Control-Max-Age";
  if (config->has("max-age") && !hdrs->has(name))
    add(name, config->getAsString("max-age"));
}


bool CORSHandler::operator()(const CtxPtr &ctx) {
  // Add headers
  Headers::set(ctx->getRequest());

  // Copy Origin if it matches
  auto &req = ctx->getRequest();
  if (req.inHas("Origin")) {
    string origin = String::toLower(req.inGet("Origin"));

    bool match = origins.find(origin) != origins.end();
    for (auto &pattern: patterns) {
      LOG_DEBUG(5, "CORS matching '" << origin << "' against '"
        << pattern << "'");
      if (match || (match = pattern.match(origin))) break;
    }

    if (match) req.outSet("Access-Control-Allow-Origin", req.inGet("Origin"));
  }

  // OPTIONS are done here
  if (req.getMethod() == HTTP::Method::HTTP_OPTIONS) {
    req.reply();
    return true;
  }

  return false;
}
