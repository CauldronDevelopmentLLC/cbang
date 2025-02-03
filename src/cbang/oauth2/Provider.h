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

#pragma once

#include <cbang/SmartPointer.h>
#include <cbang/json/Value.h>
#include <cbang/http/Method.h>
#include <cbang/http/Client.h>

#include <string>
#include <functional>


namespace cb {
  class URI;
  class Options;
  namespace JSON {class Value;}
  namespace HTTP {class Client; class Session;}

  namespace OAuth2 {
    class Provider : public HTTP::Method {
      std::string authURL;
      std::string tokenURL;
      std::string profileURL;
      std::string scope;

      std::string redirectBase;
      std::string clientID;
      std::string clientSecret;

      HTTP::Client::RequestPtr pr;

    public:
      Provider(const std::string &authURL = "",
               const std::string &tokenURL = "",
               const std::string &profileURL = "",
               const std::string &scope = "");
      virtual ~Provider();

      virtual void addOptions(Options &options);

      bool isConfigured() const;

      virtual const char *getName() const = 0;

      typedef std::function<void (const JSON::ValuePtr &)> profile_cb_t;
      void verify(HTTP::Client &client, const HTTP::Session &session,
                  const URI &uri, profile_cb_t done);

      virtual URI getRedirectURL(
        const std::string &path, const std::string &state) const;
      virtual bool isForgery(const URI &uri, const std::string &state) const;
      virtual URI getVerifyURL(const URI &uri, const std::string &state) const;
      virtual URI getProfileURL(const std::string &token) const;

      virtual std::string verifyToken(const std::string &data) const;
      virtual std::string verifyToken(
        const SmartPointer<JSON::Value> &json) const;

      virtual SmartPointer<JSON::Value> processProfile(
        const SmartPointer<JSON::Value> &profile) const = 0;

    protected:
      void validateOption(
        const std::string &option, const std::string &name) const;
    };
  }
}
