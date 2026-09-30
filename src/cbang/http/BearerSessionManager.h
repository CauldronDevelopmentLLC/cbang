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

#pragma once

#include "SessionManager.h"

#include <map>
#include <vector>


namespace cb {
  class KeyPair;

  namespace HTTP {
    // Also accepts ``Bearer <JWT>`` session IDs, as sent in an
    // ``Authorization`` header.  A JWT signed with RS256 by one of the public
    // keys gets a session whose ID is the token's SHA-256, whose user is its
    // ``sub`` claim and whose groups are its ``groups`` claim.  The session is
    // cached until the token's ``exp``, if any.  A token is revoked only by
    // rotating the signing key.  Other session IDs are as for SessionManager.
    class BearerSessionManager : public SessionManager {
      std::vector<SmartPointer<KeyPair>> keys;

      struct Entry {
        uint64_t expires; // Zero for never
        SmartPointer<Session> session;
      };

      mutable std::map<std::string, Entry> cache; // By the token's SHA-256

    public:
      BearerSessionManager();
      ~BearerSessionManager();

      // One or more PEM encoded public keys
      void addPublicKeys(const std::string &pem);
      void readPublicKeys(const std::string &path);

      // From SessionManager
      bool hasSession(const std::string &sid) const override;
      SmartPointer<Session>
      lookupSession(const std::string &sid) const override;

    protected:
      Entry verify(const std::string &token) const;
    };
  }
}
