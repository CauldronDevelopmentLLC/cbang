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

#include "SessionHandler.h"

#include <cbang/api/API.h>
#include <cbang/api/SessionQuery.h>
#include <cbang/api/QueryDef.h>
#include <cbang/api/Resolver.h>

#include <cbang/log/Logger.h>
#include <cbang/http/SessionManager.h>
#include <cbang/http/ConnIn.h>
#include <cbang/http/Server.h>

using namespace cb::API;
using namespace cb;
using namespace std;


SessionHandler::SessionHandler(API &api, const JSON::ValuePtr &config) :
  QueryHandler(api, config) {}


void SessionHandler::createSession(HTTP::Request &req) {
  // Create a session
  auto session = api.getSessionManager().openSession(req.getClientAddr());
  req.setSession(session);

  // Set session cookie
  string cookie = api.getSessionManager().getSessionCookie();
  string sid = session->getID();
  req.setCookie(cookie, sid, "", "/", 0, 0, false, true, "None");
}


bool SessionHandler::operator()(HTTP::Request &req) {
  // Check if Session is already loaded
  if (req.getSession().isSet()) return false;

  // Check if we have a session ID
  string cookie = api.getSessionManager().getSessionCookie();
  string sid    = req.getSessionID(cookie);
  if (sid.empty()) {
    createSession(req);
    return false;
  }

  // Lookup Session in SessionManager
  if (api.getSessionManager().hasSession(sid)) {
    req.setSession(api.getSessionManager().lookupSession(sid));
    return false;
  }

  // Create session
  auto session = SmartPtr(new HTTP::Session(sid, req.getClientAddr()));
  req.setSession(session);
  api.getSessionManager().addSession(session);

  // Lookup Session in DB
  if (queryDef->sql.empty()) return false;

  auto cb = [this, session, &req] (
    HTTP::Status status, const JSON::ValuePtr &result) {
    if (status == HTTP_OK) {
      if (session->hasString("user")) {
        LOG_DEBUG(3, "Authenticated: " << session->getString("user"));
        session->addGroup("authenticated");
      }

      // Restart API request processing
      req.getConnection().cast<HTTP::ConnIn>()->getServer().dispatch(req);

    } else reply(req, status, result);
  };

  auto query = SmartPtr(new SessionQuery(queryDef, req.getSession(), cb));
  query->exec(Resolver(api, req).resolve(queryDef->getSQL(), true));

  return true;
}
