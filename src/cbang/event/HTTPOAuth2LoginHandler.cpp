/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#include "HTTPOAuth2LoginHandler.h"

#include "Request.h"

#include <cbang/auth/OAuth2.h>
#include <cbang/net/SessionManager.h>
#include <cbang/log/Logger.h>
#include <cbang/json/Value.h>

using namespace cb::Event;
using namespace cb;
using namespace std;


HTTPOAuth2LoginHandler::HTTPOAuth2LoginHandler
(Client &client, const SmartPointer<OAuth2> &oauth2,
 const SmartPointer<SessionManager> &sessionManager) :
  OAuth2Login(client, oauth2), sessionManager(sessionManager) {}


HTTPOAuth2LoginHandler::~HTTPOAuth2LoginHandler() {}


void HTTPOAuth2LoginHandler::processProfile(Request &req,
                                            const JSON::ValuePtr &profile) {
  if (profile.isNull()) {
    req.reply(HTTP_UNAUTHORIZED);
    return;
  }

  // Fill session
  Session &session = *req.getSession();
  session.insert("provider",    profile->getString("provider"));
  session.insert("provider_id", profile->getString("id"));
  session.insert("name",        profile->getString("name"));
  session.insert("avatar",      profile->getString("avatar"));

  // Authenticate Session
  string email = profile->getString("email");
  session.setUser(email);
  session.addGroup("authenticated");
  LOG_INFO(1, "Authenticated: " << email);

  // Redirect the client to same path with out query params.
  req.redirect(req.getURI().getPath());
}


bool HTTPOAuth2LoginHandler::operator()(Request &req) {
  req.setCache(0); // Don't cache

  if (getOAuth2().isNull() || !getOAuth2()->isConfigured()) {
    req.reply(HTTP_SERVICE_UNAVAILABLE, "OAuth2 login not configured");
    return true;
  }

  // Get Session
  SmartPointer<Session> session = req.getSession();
  if (session.isNull()) {
    // Open new Session
    session = sessionManager->openSession(req.getClientIP());
    // Set cookie so we can pass anti-forgery test
    const string &cookie = sessionManager->getSessionCookie();
    req.setCookie(cookie, session->getID(), "", "/");

    LOG_DEBUG(3, "Opened new login session " << session->getID() << " for "
              << req.getClientIP() << " with cookie=" << cookie);

  } else if (session->hasGroup("authenticated")) return false; // Logged in

  if (req.getURI().has("state") &&
      OAuth2Login::requestToken
      (req, session->getID(), session->getString("redirect_uri", "")))
    return true;

  URI redirectURI =
    getOAuth2()->getRedirectURL(req.getURI().getPath(), session->getID());

  // Allow caller to override redirect URI
  JSON::Value &args = req.parseArgs();
  if (args.hasString("redirect_uri")) {
    string uri = args.getString("redirect_uri");
    redirectURI.set("redirect_uri", uri);
    session->insert("redirect_uri", uri);
  }

  req.redirect(redirectURI);

  return true;
}
