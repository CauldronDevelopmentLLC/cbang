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

#include "ACLHandler.h"
#include "Request.h"

#include <cbang/util/ACLSet.h>
#include <cbang/log/Logger.h>

#include <string>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


bool ACLHandler::operator()(Request &req) {
  string path = req.getURI().getPath();
  string user = req.getUser();
  string group;
  bool allow;

  if (user.empty())
    allow = aclSet.allowGroup(path, group = "unauthenticated");

  else {
    allow = aclSet.allow(path, user);
    if (!allow) allow = aclSet.allowGroup(path, group = "authenticated");
    if (!allow) allow = aclSet.allowGroup(path, group = "unauthenticated");
  }

  if (!allow && req.getSession().isSet()) {
    auto groups = req.getSession()->getGroups();
    for (auto &g: groups)
      if (aclSet.allowGroup(path, group = g)) {allow = true; break;}
  }

  LOG_INFO(allow ? 5 : 3, "allow(" << path << ", "
           << (user.empty() ? "@" + group : user) << ", "
           << req.getClientAddr() << ") = "
           << (allow ? "true" : "false"));

  if (!allow) {
    if (cb) cb(req);
    else req.reply(HTTP_UNAUTHORIZED);
    return true;
  }

  return false;
}
