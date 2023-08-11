/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include "HTTPAccessHandler.h"

#include "Request.h"

#include <cbang/log/Logger.h>
#include <cbang/net/Session.h>

using namespace std;
using namespace cb;
using namespace cb::Event;


bool HTTPAccessHandler::userAllow(const string &name) const {
  return userAllowed.find(name) != userAllowed.end();
}


bool HTTPAccessHandler::userDeny(const string &name) const {
  return userDenied.find(name) != userDenied.end();
}


bool HTTPAccessHandler::groupAllow(const string &name) const {
  return groupAllowed.find(name) != groupAllowed.end();
}


bool HTTPAccessHandler::groupDeny(const string &name) const {
  return groupDenied.find(name) != groupDenied.end();
}


bool HTTPAccessHandler::operator()(Request &req) {
  SmartPointer<Session> session = req.getSession();
  string user = req.getUser();
  bool allow;
  bool deny;

  if (session.isNull()) {
    user  = "@unauthenticted";
    allow = groupAllow("unauthenticted");
    deny  = groupDeny("unauthenticted");

  } else {

    allow = userAllow(user);
    deny = userDeny(user);

    auto &groups = *session->get("group");
    for (unsigned i = 0; i < groups.size(); i++)
      if (groups.getBoolean(i)) {
        string group = groups.keyAt(i);
        if (!allow) allow = groupAllow(group);
        if (!deny) deny = groupDeny(group);
      }

    if (!allow) allow = groupAllow("authenticated");
    if (!deny)   deny = groupDeny("authenticated");
  }

  LOG_INFO(allow ? 5 : 3, "allow(" << req.getURI().getPath() << ", "
           << user << ", " << req.getClientIP().getHost() << ") = "
           << ((allow && !deny) ? "true" : "false"));

  if (!allow || deny) THROWX("Access denied", HTTP_UNAUTHORIZED);

  return false;
}
