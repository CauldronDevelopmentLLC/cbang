/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2017, Cauldron Development LLC
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

#pragma once

#include "RequestMethod.h"
#include "HTTPStatus.h"

#include <cbang/SmartPointer.h>

#include <string>


namespace cb {
  class OAuth2;
  namespace JSON {class Value;}

  namespace Event {
    class Client;
    class Request;

    class OAuth2Login : RequestMethod, HTTPStatus {
      Client &client;
      SmartPointer<OAuth2> oauth2;

    public:
      OAuth2Login(Client &client, const SmartPointer<OAuth2> &oauth2 = 0);
      virtual ~OAuth2Login();

      const SmartPointer<OAuth2> &getOAuth2() {return oauth2;}
      void setOAuth2(const SmartPointer<OAuth2> &oauth2)
      {this->oauth2 = oauth2;}

      virtual void processProfile(Request &req,
                                  const SmartPointer<JSON::Value> &profile) = 0;

      bool authorize(Request &req, const std::string &state);
      bool authRedirect(Request &req, const std::string &state);
      bool requestToken(Request &req, const std::string &state,
                        const std::string &redirect_uri = std::string());

    protected:
      void verifyToken(Request &req, const std::string &response);
    };
  }
}
