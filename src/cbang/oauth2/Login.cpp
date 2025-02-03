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

#include "Login.h"
#include "Provider.h"

#include <cbang/http/Request.h>
#include <cbang/http/Client.h>
#include <cbang/Catch.h>
#include <cbang/net/URI.h>
#include <cbang/json/Value.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::OAuth2;


Login::Login(HTTP::Client &client, const SmartPointer<Provider> &provider) :
  client(client), provider(provider) {}
Login::~Login() {}


bool Login::requestToken(HTTP::Request &req, const string &state,
                         const string &redirectURI) {
  auto &uri = req.getURI();
  if (provider->isForgery(uri, state)) {
    LOG_DEBUG(3, "Failed anti-forgery check: uri code="
              << (uri.has("code") ? uri.get("code") : "<null>") << " uri state="
              << (uri.has("state") ? uri. get("state") : "<null>")
              << " server state=" << state);

    return false;
  }

  URI verifyURL = provider->getVerifyURL(uri, state);

  // Override redirect URI
  if (!redirectURI.empty()) verifyURL.set("redirect_uri", redirectURI);

  // Extract query data
  string data = verifyURL.getQuery();
  verifyURL.setQuery("");

  // Verify authorization with OAuth2 server
  auto cb = [this, &req] (HTTP::Request &_req) {
    verifyToken(req, _req.getInput());
  };
  pr = client.call(verifyURL, HTTP_POST, data, cb);
  pr->getRequest()->setContentType("application/x-www-form-urlencoded");
  pr->getRequest()->outSet("Accept", "application/json");
  pr->send();

  return true;
}


void Login::verifyToken(HTTP::Request &req, const string &response) {
  auto handler =
    [this, &req] (HTTP::Request &_req) {
      if (_req.isOk())
        try {
          auto profile = provider->processProfile(_req.getInputJSON());
          LOG_DEBUG(3, "OAuth2 Profile: " << *profile);
          return processProfile(req, profile);
        } CATCH_ERROR;

      processProfile(req, 0);
      pr.release();
    };

  if (!response.empty())
    try {
      // Verify
      string accessToken = provider->verifyToken(response);
      auto profileURL    = provider->getProfileURL(accessToken);

      // Get profile
      pr = client.call(profileURL, HTTP_GET, handler);
      pr->getRequest()->outSet("User-Agent", "cbang.org");
      return pr->send();

    } catch (const Exception &e) {
      LOG_ERROR("Login verification failed: " << response);
    }

  processProfile(req, 0); // Notify incase of error
}
