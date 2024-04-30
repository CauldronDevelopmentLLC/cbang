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

#include "AccessHandler.h"
#include "Request.h"
#include "Session.h"

#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


void AccessHandler::read(const JSON::ValuePtr &config) {
  if (config->has("allow")) read(config->get("allow"), true);
  if (config->has("deny"))  read(config->get("deny"),  false);
}


void AccessHandler::read(const JSON::ValuePtr &config, bool allow) {
  if (config->isList())
    for (unsigned i = 0; i < config->size(); i++)
      read(config->get(i), allow);

  else {
    string name = config->asString();

    if (name.empty()) return;
    if (name == "*") setGroup("*", allow);
    if (name[0] == '$' || name[0] == '@') setGroup(name.substr(1), allow);
    else setUser(name, allow);
  }
}


void AccessHandler::setUser(const std::string &name, bool allow) {
  if (allow) allowUser(name);
  else denyUser(name);
}


void AccessHandler::setGroup(const std::string &name, bool allow) {
  if (allow) allowGroup(name);
  else denyGroup(name);
}


bool AccessHandler::userAllow(const string &name) const {
  return userAllowed.find(name) != userAllowed.end();
}


bool AccessHandler::userDeny(const string &name) const {
  return userDenied.find(name) != userDenied.end();
}


bool AccessHandler::groupAllow(const string &name) const {
  return groupAllowed.find(name) != groupAllowed.end();
}


bool AccessHandler::groupDeny(const string &name) const {
  return groupDenied.find(name) != groupDenied.end();
}


bool AccessHandler::checkGroup(
  const string &name, bool &allow, bool &deny) const {
  bool matched = false;

  if (!allow) {
    allow = groupAllow(name);
    if (allow) matched = true;
  }

  if (!deny) {
    deny = groupDeny(name);
    if (deny) matched = true;
  }

  return matched;
}


bool AccessHandler::operator()(Request &req) {
  SmartPointer<Session> session = req.getSession();
  string user = req.getUser();
  string group;
  bool allow = false;
  bool deny  = false;

  if (checkGroup("*", allow, deny)) group = "@*";

  if (session.isNull()) {
    user = "anonymous";
    if (checkGroup("unauthenticated", allow, deny)) group = "@unauthenticated";

  } else {
    allow = userAllow(user);
    deny  = userDeny(user);

    auto &groups = *session->get("group");
    for (unsigned i = 0; i < groups.size(); i++)
      if (groups.getBoolean(i)) {
        auto &name = groups.keyAt(i);
        if (checkGroup(name, allow, deny)) group = "@" + name;
      }

    if (checkGroup("authenticated", allow, deny)) group = "@authenticated";
  }

  LOG_INFO(allow ? 5 : 3, "allow(" << req.getURI().getPath() << ", "
           << user << ", " << group << ", " << req.getClientAddr() << ") = "
           << ((allow && !deny) ? "true" : "false"));

  if (!allow || deny) THROWX("Access denied", HTTP_UNAUTHORIZED);

  return false;
}
