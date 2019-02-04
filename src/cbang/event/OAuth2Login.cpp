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

#include "OAuth2Login.h"
#include "Request.h"
#include "PendingRequest.h"
#include "Client.h"

#include <cbang/auth/OAuth2.h>
#include <cbang/net/URI.h>
#include <cbang/json/Value.h>
#include <cbang/Catch.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb::Event;


OAuth2Login::OAuth2Login(Client &client, const SmartPointer<OAuth2> &oauth2) :
  client(client), oauth2(oauth2) {}
OAuth2Login::~OAuth2Login() {}


bool OAuth2Login::authorize(Request &req, const string &state) {
  if (req.getURI().has("state")) return requestToken(req, state);
  return authRedirect(req, state);
}


bool OAuth2Login::authRedirect(Request &req, const string &state) {
  req.redirect(getOAuth2()->getRedirectURL(req.getURI().getPath(), state));
  return true;
}


bool OAuth2Login::requestToken(Request &req, const string &state,
                               const string &redirect_uri) {
  auto handler =
    [this, &req] (Request *_req, int err) {
      verifyToken(req, _req ? _req->getInput() : "");
    };

  URI verifyURL = getOAuth2()->getVerifyURL(req.getURI(), state);

  // Override redirect URI
  if (!redirect_uri.empty()) verifyURL.set("redirect_uri", redirect_uri);

  // Extract query data
  string data = verifyURL.getQuery();
  verifyURL.setQuery("");

  // Verify authorization with OAuth2 server
  SmartPointer<PendingRequest> pr =
    client.call(verifyURL, HTTP_POST, data, handler);
  pr->setContentType("application/x-www-form-urlencoded");
  pr->outSet("Accept", "application/json");
  pr->send();

  return true;
}


void OAuth2Login::verifyToken(Request &req, const string &response) {
  auto handler =
    [this, &req] (Request *_req, int err) {
      if (_req)
        try {
          SmartPointer<JSON::Value> profile =
            getOAuth2()->processProfile(_req->getInputJSON());

          LOG_DEBUG(3, "OAuth2 Profile: " << *profile);
          processProfile(req, profile);
          return;
        } CATCH_ERROR;

      processProfile(req, 0);
    };

  if (!response.empty())
    try {
      // Verify
      string accessToken = getOAuth2()->verifyToken(response);

      // Get profile
      SmartPointer<PendingRequest> pr =
        client.call(getOAuth2()->getProfileURL(accessToken), HTTP_GET, handler);
      pr->outSet("User-Agent", "cbang.org");
      pr->send();

      return;
    } catch (const Exception &e) {
      LOG_ERROR("OAuth2Login verification failed: " << response);
    }

  processProfile(req, 0); // Notify incase of error
}
