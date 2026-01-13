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

#include "Login.h"
#include "QueryDef.h"
#include "API.h"

#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/http/Request.h>
#include <cbang/api/Resolver.h>

using namespace std;
using namespace cb;
using namespace cb::API;


Login::Login(const QueryDef &def, callback_t cb,
  const ResolverPtr &resolver, HTTP::Request &req, const string &provider,
  const string &redirectURI) : Query(def, cb), resolver(resolver),
  provider(provider), redirectURI(redirectURI), session(req.getSession()),
  uri(req.getURI()) {}


void Login::loginComplete() {
  auto origCB = cb;

  cb = [this, origCB] (HTTP::Status status, const JSON::ValuePtr &result) {
    if (status == HTTP_OK) {
      // Authenticate Session
      session->addGroup("authenticated");

      // Respond with session
      return origCB(status, session);
    }

    origCB(status, result);
  };

  if (def.sql.empty()) cb(HTTP_OK, 0);
  else exec(resolver->resolve(def.getSQL(), true));
}


void Login::login() {
  if (provider == "none") return loginComplete();

  // Get OAuth2 provider
  auto oauth2 = def.api.getOAuth2Providers().get(provider);
  if (oauth2.isNull() || !oauth2->isConfigured())
    return errorReply(
      HTTP_BAD_REQUEST, "Unsupported login provider: " + provider);

  // Handle OAuth login response
  if (uri.has("state")) {
    if (!redirectURI.empty()) session->insert("redirect_uri", redirectURI);
    auto self = SmartPtr(this);
    auto cb = [self] (const JSON::ValuePtr &profile) {
      self->processProfile(profile);
    };
    return oauth2->verify(def.api.getClient(), *session, uri, cb);
  }

  // Respond with Session ID and redirect URL
  string sid = session->getID();
  URI redirect = oauth2->getRedirectURL(uri.getPath(), sid);
  if (!redirectURI.empty()) {
    redirect.set("redirect_uri", redirectURI);
    session->insert("redirect_uri", redirectURI);
  }

  sink.beginDict();
  sink.insert("id", sid);
  sink.insert("redirect", redirect);
  sink.endDict();

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
    if (resultCount) session->addGroup(db->getString(0)); // Groups
    else if (db->getField(1).isNumber()) // Session vars
      session->insert(db->getString(0), db->getDouble(1));
    else session->insert(db->getString(0), db->getString(1));
    break;
  }

  default: return returnOk(state);
  }
}
