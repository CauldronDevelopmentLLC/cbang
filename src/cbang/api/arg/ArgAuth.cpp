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

#include "ArgAuth.h"

#include <cbang/String.h>
#include <cbang/http/Request.h>

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

  for (unsigned i = 0; i < config->size(); i++) {
    string constraint = config->getString(i);
    if (constraint.empty()) THROW("Empty arg" << type << " constraint");

    if (constraint[0] == '$') groups.push_back(constraint.substr(1));
    else if (constraint[0] == '=')
      sessionVars.push_back(constraint.substr(1));
    else THROW("Invalid arg " << type << " constraint.  Must start with "
                "'$' for group or '=' for session variable.");
  }
}




void ArgAuth::operator()(HTTP::Request &req, JSON::Value &_value) const {
  SmartPointer<HTTP::Session> session = req.getSession();
  if (session.isNull()) unauthorized();

  for (unsigned i = 0; i < groups.size(); i++)
    if (session->hasGroup(groups[i])) {
      if (allow) return;
      unauthorized();
    }

  string value = _value.asString();
  for (unsigned i = 0; i < sessionVars.size(); i++)
    if (session->has(sessionVars[i]) &&
        session->getAsString(sessionVars[i]) == value) {
      if (allow) return;
      unauthorized();
    }

  if (allow) unauthorized();
}
