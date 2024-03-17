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

#include "Login.h"
#include "API.h"

#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/http/Request.h>

using namespace std;
using namespace cb;
using namespace cb::API;


Login::Login(
  API &api, const SmartPointer<HTTP::Request> &req, const string &sql,
  const string &provider, const string &redirectURI) :
  Query(api, req, sql), api(api), provider(provider),
  redirectURI(redirectURI) {}


void Login::loginComplete() {
  auto origCB = cb;

  auto cb = [this, origCB] (HTTP::Status status, Event::Buffer &buffer) {
    if (status == HTTP_OK) {
      // Authenticate Session
      auto session = req->getSession();
      session->addGroup("authenticated");

      // Respond with session
      session->write(writer);
    }

    writer.close();
    origCB(status, writer);
  };

  if (sql.empty()) cb(HTTP_OK, writer);
  else query(cb);
}


void Login::login(callback_t cb) {
  this->cb = cb;

  // Make sure we have Session
  auto session = req->getSession();
  if (session.isNull()) {
    session = api.getSessionManager().openSession(req->getClientAddr());
    req->setSession(session);
  }

  // Set session cookie
  string sid    = session->getID();
  string cookie = api.getSessionManager().getSessionCookie();
  req->setCookie(cookie, sid, "", "/", 0, 0, false, true, "None");

  if (provider == "none") return loginComplete();

  // Get OAuth2 provider
  auto oauth2 = api.getOAuth2Providers().get(provider);
  if (oauth2.isNull() || !oauth2->isConfigured())
    return errorReply(
      HTTP_BAD_REQUEST, "Unsupported login provider: " + provider);

  // Handle OAuth login response
  const URI &uri = req->getURI();
  if (uri.has("state")) {
    if (!redirectURI.empty()) session->insert("redirect_uri", redirectURI);
    auto cb = [this] (const JSON::ValuePtr &profile) {processProfile(profile);};
    return oauth2->verify(api.getClient(), *session, uri, cb);
  }

  // Respond with Session ID and redirect URL
  URI redirect = oauth2->getRedirectURL(uri.getPath(), sid);
  if (!redirectURI.empty()) {
    redirect.set("redirect_uri", redirectURI);
    session->insert("redirect_uri", redirectURI);
  }

  writer.beginDict();
  writer.insert("id", sid);
  writer.insert("redirect", redirect);
  writer.endDict();
  writer.close();

  reply(HTTP_OK);
}


void Login::processProfile(const JSON::ValuePtr &profile) {
  if (profile.isSet()) {
    try {
      string provider = profile->getString("provider");

      // Fix up Facebook avatar
      if (provider == "facebook")
        profile->insert("avatar", "http://graph.facebook.com/" +
                        profile->getString("id") + "/picture?type=small");

      // Fix up for GitHub name
      if ((!profile->hasString("name") ||
           String::trim(profile->getString("name")).empty()) &&
          profile->hasString("login"))
        profile->insert("name", profile->getString("login"));

      // Fill session
      auto session = req->getSession();
      session->setUser(profile->getString("email"));
      session->insert("provider",    provider);
      session->insert("provider_id", profile->getString("id"));
      session->insert("name",        profile->getString("name"));
      session->insert("avatar",      profile->getString("avatar"));

      return loginComplete();
    } CATCH_ERROR;

    LOG_ERROR("Invalid login profile: " << *profile);
  }

  errorReply(HTTP_UNAUTHORIZED);
}


void Login::callback(state_t state) {
  switch (state) {
  case MariaDB::EventDB::EVENTDB_END_RESULT: resultCount++; break;

  case MariaDB::EventDB::EVENTDB_ROW: {
    auto session = req->getSession();

    if (resultCount) session->addGroup(db->getString(0)); // Groups
    else if (db->getField(1).isNumber()) // Session vars
      session->insert(db->getString(0), db->getDouble(1));
    else session->insert(db->getString(0), db->getString(1));
    break;
  }

  default: return returnOk(state);
  }
}
