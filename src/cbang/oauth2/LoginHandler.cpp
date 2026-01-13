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

#include "LoginHandler.h"
#include "Provider.h"

#include <cbang/http/Request.h>
#include <cbang/http/SessionManager.h>
#include <cbang/log/Logger.h>
#include <cbang/json/Value.h>

using namespace cb::OAuth2;
using namespace cb;
using namespace std;


LoginHandler::LoginHandler(
  HTTP::Client &client, const SmartPointer<Provider> &provider,
  const SmartPointer<HTTP::SessionManager> &sessionManager) :
  Login(client, provider), sessionManager(sessionManager) {}


LoginHandler::~LoginHandler() {}


void LoginHandler::processProfile(
  HTTP::Request &req, const JSON::ValuePtr &profile) {
  if (profile.isNull()) return req.reply(HTTP_UNAUTHORIZED);

  // Fill session
  auto &session = *req.getSession();
  session.insert("provider",    profile->getString("provider"));
  session.insert("provider_id", profile->getString("id"));
  session.insert("name",        profile->getString("name"));
  session.insert("avatar",      profile->getString("avatar"));

  // Authenticate Session
  string email = profile->getString("email");
  session.setUser(email);
  session.addGroup("authenticated");
  LOG_INFO(2, "Authenticated: " << email);

  // Redirect the client to same path with out query params.
  req.redirect(req.getURI().getPath());
}


bool LoginHandler::operator()(HTTP::Request &req) {
  req.setCache(0); // Don't cache

  if (getProvider().isNull() || !getProvider()->isConfigured()) {
    req.reply(HTTP_SERVICE_UNAVAILABLE, "OAuth2 login not configured");
    return true;
  }

  // Get Session
  auto session = req.getSession();
  if (session.isNull()) {
    // Open new Session
    session = sessionManager->openSession(req.getClientAddr());

    // Set cookie so we can pass anti-forgery test
    const string &cookie = sessionManager->getSessionCookie();
    req.setCookie(cookie, session->getID(), "", "/");

    LOG_DEBUG(3, "Opened new login session " << session->getID() << " for "
              << req.getClientAddr() << " with cookie=" << cookie);

  } else if (session->hasGroup("authenticated")) return false; // Logged in

  if (req.getURI().has("state") &&
      Login::requestToken(
        req, session->getID(), session->getString("redirect_uri", "")))
    return true;

  URI redirectURI =
    getProvider()->getRedirectURL(req.getURI().getPath(), session->getID());

  // Allow caller to override redirect URI
  JSON::Value &args = *req.parseArgs();
  if (args.hasString("redirect_uri")) {
    string uri = args.getString("redirect_uri");
    redirectURI.set("redirect_uri", uri);
    session->insert("redirect_uri", uri);

  } else if (session->hasString("redirect_uri"))
    redirectURI.set("redirect_uri", session->getString("redirect_uri"));

  req.redirect(redirectURI);

  return true;
}
