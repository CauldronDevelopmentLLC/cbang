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

#include <cbang/config.h>
#include <cbang/util/Singleton.h>
#include <cbang/net/IPAddress.h>

#include <string>

namespace cb {
  class Options;

  namespace HTTP {
    class Request;
    class Response;
    class Transaction;

    class ProxyManager : public Singleton<ProxyManager> {
      mutable IPAddress address;
      bool enable;
      std::string proxy;
      std::string user;
      std::string pass;

      bool initialized;
      std::string scheme;
      std::string realm;
      std::string nonce;
#ifdef HAVE_OPENSSL // clang complains about unused member
      unsigned nonceCount;
#endif
      std::string cnonce;
      std::string opaque;
      std::string algorithm;
      std::string qop;

      ~ProxyManager() {}

    public:
      ProxyManager(Inaccessible);

      void addOptions(Options &options);

      bool enabled() const {return enable && !proxy.empty();}
      bool hasCredentials() const {return !user.empty();}
      const IPAddress &getAddress() const;
      const std::string &getUser() const {return user;}
      const std::string &getPassword() const {return pass;}

      void connect(Transaction &transaction);
      void authenticate(Response &response);
      void addCredentials(Request &request);
    };
  }
}
