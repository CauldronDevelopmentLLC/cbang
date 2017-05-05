/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2015, Cauldron Development LLC
                 Copyright (c) 2003-2015, Stanford University
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

#ifndef CB_EVENT_DNSBASE_H
#define CB_EVENT_DNSBASE_H

#include "DNSCallback.h"
#include "DNSRequest.h"

#include <cbang/SmartPointer.h>

#include <string>
#include <set>

struct evdns_base;


namespace cb {
  namespace Event {
    typedef enum {
      DNS_ERR_NONE = 0,
      DNS_ERR_FORMAT = 1,
      DNS_ERR_SERVERFAILED = 2,
      DNS_ERR_NOTEXIST = 3,
      DNS_ERR_NOTIMPL = 4,
      DNS_ERR_REFUSED = 5,
      DNS_ERR_TRUNCATED = 65,
      DNS_ERR_UNKNOWN = 66,
      DNS_ERR_TIMEOUT = 67,
      DNS_ERR_SHUTDOWN = 68,
      DNS_ERR_CANCEL = 69,
      DNS_ERR_NODATA = 70,
    } dns_error_t;

    class Base;
    class DNSRequest;

    class DNSBase {
      evdns_base *dns;

      bool failRequestsOnExit;

    public:
      DNSBase(Base &base, bool initialize = true,
              bool failRequestsOnExit = true);
      ~DNSBase();

      evdns_base *getDNSBase() const {return dns;}

      void addNameserver(const IPAddress &ns);

      DNSRequest resolve(const std::string &name,
                         const SmartPointer<DNSCallback> &cb,
                         bool search = true);
      DNSRequest reverse(uint32_t ip, const SmartPointer<DNSCallback> &cb,
                         bool search = true);

      template <class T>
      DNSRequest resolve(const std::string &name, T *obj,
                         typename DNSMemberFunctor<T>::member_t member,
                         bool search = true) {
        return resolve(name, new DNSMemberFunctor<T>(obj, member), search);
      }

      template <class T>
      DNSRequest reverse(uint32_t ip, T *obj,
                         typename DNSMemberFunctor<T>::member_t member,
                         bool search = true) {
        return reverse(ip, new DNSMemberFunctor<T>(obj, member), search);
      }

      static const char *getErrorStr(int error);
    };
  }
}

#endif // CB_EVENT_DNSBASE_H
