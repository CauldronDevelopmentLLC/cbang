/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include "SessionHandler.h"

#include <cbang/api/API.h>
#include <cbang/api/SessionQuery.h>
#include <cbang/log/Logger.h>
#include <cbang/http/SessionManager.h>
#include <cbang/http/ConnIn.h>
#include <cbang/http/Server.h>

using namespace cb::API;
using namespace cb;
using namespace std;


SessionHandler::SessionHandler(API &api, const JSON::ValuePtr &config) :
  QueryHandler(api, config) {}


bool SessionHandler::operator()(HTTP::Request &req) {
  auto &sessionMan = api.getSessionManager();

  // Check if Session is already loaded
  if (req.getSession().isSet()) return false;

  // Check if we have a session ID
  string sid = req.getSessionID(sessionMan.getSessionCookie());
  if (sid.empty()) return false;

  // Lookup Session in SessionManager
  try {
    req.setSession(sessionMan.lookupSession(sid));
    LOG_DEBUG(3, "User: " << req.getUser());

    return false;
  } catch (const Exception &) {}

  // Lookup Session in DB
  if (sql.empty()) return false;
  auto session = SmartPtr(new HTTP::Session(sid, req.getClientAddr()));
  req.setSession(session);

  auto query = SmartPtr(new SessionQuery(api, &req, sql));

  auto cb = [this, query, session, &req] (
    HTTP::Status status, Event::Buffer &buffer) {
    if (status == HTTP_OK) {
      if (session->hasString("user")) {
        // Session was found in DB
        session->addGroup("authenticated");
        api.getSessionManager().addSession(session);
      }

      // Restart API request processing
      req.getConnection().cast<HTTP::ConnIn>()->getServer().dispatch(req);

    } else reply(req, status, buffer);
  };

  query->query(cb);

  return true;
}
