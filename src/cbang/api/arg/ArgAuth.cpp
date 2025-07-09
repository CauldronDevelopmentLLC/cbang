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

#include "ArgAuth.h"

#include <cbang/String.h>
#include <cbang/http/Status.h>

using namespace cb::API;
using namespace cb;
using namespace std;


namespace {
  void unauthorized() {
    THROWX("Unauthorized", HTTP::Status::HTTP_UNAUTHORIZED);
  }
}


ArgAuth::ArgAuth(bool allow, const JSON::ValuePtr &config) : allow(allow) {
  const char *type = allow ? "allow" : "deny";

  for (auto &value: *config) {
    string constraint = value->getString();
    if (constraint.empty()) THROW("Empty arg" << type << " constraint");

    if (constraint[0] == '$') groups.push_back(constraint.substr(1));
    else if (constraint[0] == '=')
      sessionVars.push_back(constraint.substr(1));
    else THROW("Invalid arg " << type << " constraint.  Must start with "
                "'$' for group or '=' for session variable.");
  }
}


JSON::ValuePtr ArgAuth::operator()(
  const CtxPtr &ctx, const JSON::ValuePtr &value) const {

  auto session = ctx->getRequest().getSession();
  if (session.isNull()) unauthorized();

  for (auto &group: groups)
    if (session->getBoolean(group, false)) {
      if (allow) return value;
      unauthorized();
    }

  string s = value->asString();
  for (auto &var: sessionVars)
    if (session->has(var) && session->getAsString(var) == s) {
      if (allow) return value;
      unauthorized();
    }

  if (allow) unauthorized();

  return value;
}
