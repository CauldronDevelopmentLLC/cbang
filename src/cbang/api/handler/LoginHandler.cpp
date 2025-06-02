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

#include "LoginHandler.h"

#include <cbang/api/API.h>
#include <cbang/api/Login.h>

using namespace std;
using namespace cb;
using namespace cb::API;


LoginHandler::LoginHandler(API &api, const JSON::ValuePtr &config) :
  QueryHandler(api, config), provider(config->getString("provider", "")),
  redirect(config->getString("redirect", "")) {}


void LoginHandler::reportSession(HTTP::Request &req) {
  auto session = req.getSession();

  if (session.isNull() || !session->hasGroup("authenticated"))
    req.sendJSONError(HTTP_UNAUTHORIZED, "Not logged in");

  else {
    // Respond with session JSON
    session->write(*req.getJSONWriter());
    req.reply();
  }
}


void LoginHandler::listProviders(HTTP::Request &req) {
  auto &providers = api.getOAuth2Providers();
  auto writer     = req.getJSONWriter();

  writer->beginList();
  for (auto &p: providers)
    if (p.second->isConfigured()) writer->append(p.first);
  writer->endList();

  writer.release();
  req.reply();
}


bool LoginHandler::operator()(HTTP::Request &req) {
  auto &args         = *req.getArgs();
  string provider    = args.getString("provider", "");
  string redirectURI = args.getString("redirect_uri", "");

  if (!this->provider.empty()) provider = this->provider;

  if (provider.empty()) reportSession(req);
  else if (provider == "providers") listProviders(req);

  else {
    auto cb = [this, &req] (HTTP::Status status, const JSON::ValuePtr &result) {
      // Respond with JSON or redirect
      if (status != HTTP_OK || redirect.empty()) reply(req, status, result);
      else req.redirect(redirect);
    };

    auto login = SmartPtr(new Login(
      queryDef, cb, new Resolver(api, req), req, provider, redirectURI));
    login->login();
  }

  return true;
}
