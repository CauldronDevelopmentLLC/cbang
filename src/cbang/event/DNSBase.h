/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include "DNSRequest.h"

struct evdns_base;


namespace cb {
  namespace Event {

    class Base;

    class DNSBase {
      evdns_base *dns;
      bool failRequestsOnExit;

    public:
      DNSBase(Base &base, bool initialize = true,
              bool failRequestsOnExit = true);
      ~DNSBase();

      evdns_base *getDNSBase() const {return dns;}

      void initSystemNameservers();

      void addNameserver(const std::string &addr);
      void addNameserver(const IPAddress &ns);

      /// Options are: ndots, timeout, max-timeouts, max-inflight, attempts,
      ///   randomize-case, bind-to, initial-probe-timeout,
      ///   getaddrinfo-allow-skew.
      void setOption(const std::string &name, const std::string &value);

      typedef std::function<void (int, std::vector<IPAddress> &, int)>
      callback_t;

      SmartPointer<DNSRequest>
      resolve(const std::string &name, DNSRequest::callback_t cb,
              bool search = true);
      SmartPointer<DNSRequest>
      reverse(uint32_t ip, DNSRequest::callback_t cb, bool search = true);
    };
  }
}
