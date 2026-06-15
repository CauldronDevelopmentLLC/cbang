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

#include "SessionHandler.h"

#include <cbang/api/API.h>
#include <cbang/api/SessionQuery.h>
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


void SessionHandler::operator()(const CtxPtr &ctx, const Cont &next) {
  // Check if Session is already loaded
  auto &req = ctx->getRequest();
  if (req.getSession().isSet()) return next(ctx);

  // Check if we have a session ID
  auto &sessionMan = api.getSessionManager();
  string cookie    = sessionMan.getSessionCookie();
  string sid       = req.getSessionID(cookie);

  if (sid.empty()) {
    // Create new session and set cookie
    auto session = sessionMan.openSession(req.getClientAddr());
    ctx->setSession(session);
    sid = session->getID();
    req.setCookie(cookie, sid, "", "/", 0, 0, false, true, "None");

    // A response that sets the session cookie must never be stored by a
    // shared cache, or the new session ID would be served to other clients
    req.outSet("Cache-Control", "no-store");

    return next(ctx);
  }

  // Lookup Session in SessionManager
  if (sessionMan.hasSession(sid)) {
    ctx->setSession(sessionMan.lookupSession(sid));
    return next(ctx);
  }

  // Create session
  auto session = SmartPtr(new HTTP::Session(sid, req.getClientAddr()));
  ctx->setSession(session);
  sessionMan.addSession(session);

  // Lookup Session in DB
  if (queryDef->sql.empty()) return next(ctx);

  auto cb = [this, session, ctx, next] (
    HTTP::Status status, const JSON::ValuePtr &result) {
    if (status == HTTP_OK) {
      if (session->hasString("user")) {
        LOG_DEBUG(3, "Authenticated: " << session->getString("user"));
        session->addGroup("authenticated");
      }

      // Continue processing with the loaded session
      next(ctx);

    } else reply(ctx, status, result);
  };

  auto query = SmartPtr(new SessionQuery(*queryDef, req.getSession(), cb));
  vector<JSON::ValuePtr> params;
  string s = ctx->getResolver()->resolveSQL(queryDef->getSQL(), params);
  query->exec(s, params);
}
