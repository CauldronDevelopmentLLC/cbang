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

#pragma once

#include "RequestHandler.h"

#include <cbang/json/Value.h>

#include <set>
#include <string>


namespace cb {
  namespace HTTP {
    class Request;

    class AccessHandler : public RequestHandler {
      std::set<std::string> userAllowed;
      std::set<std::string> userDenied;
      std::set<std::string> groupAllowed;
      std::set<std::string> groupDenied;

    public:
      AccessHandler() {}
      AccessHandler(const JSON::ValuePtr &config) {read(config);}

      void read(const JSON::ValuePtr &config);
      void read(const JSON::ValuePtr &config, bool allow);

      void setUser   (const std::string &name, bool allow);
      void allowUser (const std::string &name) {userAllowed.insert(name);}
      void denyUser  (const std::string &name) {userDenied.insert(name);}
      void setGroup  (const std::string &name, bool allow);
      void allowGroup(const std::string &name) {groupAllowed.insert(name);}
      void denyGroup (const std::string &name) {groupDenied.insert(name);}

      bool userAllow (const std::string &name) const;
      bool userDeny  (const std::string &name) const;
      bool groupAllow(const std::string &name) const;
      bool groupDeny (const std::string &name) const;

      bool checkGroup(const std::string &name, bool &allow, bool &deny) const;

      // From RequestHandler
      bool operator()(Request &req) override;
    };
  }
}
