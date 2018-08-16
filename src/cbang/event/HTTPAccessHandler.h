/******************************************************************************\

         This file is part of the C! library.  A.K.A the cbang library.

               Copyright (c) 2003-2018, Cauldron Development LLC
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

#include "HTTPHandler.h"

#include <set>
#include <string>


namespace cb {
  namespace Event {
    class HTTPAccessHandler : public HTTPHandler {
      std::set<std::string> userAllowed;
      std::set<std::string> userDenied;
      std::set<std::string> groupAllowed;
      std::set<std::string> groupDenied;

    public:
      HTTPAccessHandler() {}

      void allowUser(const std::string &name) {userAllowed.insert(name);}
      void denyUser(const std::string &name) {userDenied.insert(name);}
      void allowGroup(const std::string &name) {groupAllowed.insert(name);}
      void denyGroup(const std::string &name) {groupDenied.insert(name);}

      bool userAllow(const std::string &name) const;
      bool userDeny(const std::string &name) const;
      bool groupAllow(const std::string &name) const;
      bool groupDeny(const std::string &name) const;

      // From HTTPHandler
      bool operator()(Request &req);
    };
  }
}
