/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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
#include <cbang/http/Client.h>
#include <cbang/http/Status.h>

#include <string>


namespace cb {
  namespace JSON {class Value;}
  namespace HTTP {class Client; class Request;}

  namespace OAuth2 {
    class Provider;

    class Login : public HTTP::Method, public HTTP::Status {
      HTTP::Client &client;
      SmartPointer<Provider> provider;
      HTTP::Client::RequestPtr pr;

    public:
      Login(HTTP::Client &client, const SmartPointer<Provider> &provider = 0);
      virtual ~Login();

      const SmartPointer<Provider> &getProvider() {return provider;}
      void setProvider(const SmartPointer<Provider> &provider)
      {this->provider = provider;}

      virtual void processProfile(
        HTTP::Request &req, const SmartPointer<JSON::Value> &profile) = 0;

      bool requestToken(HTTP::Request &req, const std::string &state,
                        const std::string &redirect_uri = std::string());

    protected:
      void verifyToken(HTTP::Request &req, const std::string &response);
    };
  }
}
