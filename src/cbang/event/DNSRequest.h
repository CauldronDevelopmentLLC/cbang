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

#include <cbang/SmartPointer.h>
#include <cbang/net/IPAddress.h>

#include <functional>
#include <string>
#include <vector>

struct evdns_base;
struct evdns_request;


namespace cb {
  namespace Event {
    class DNSRequest : public RefCounted {
    public:
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

      typedef
      std::function<void (int error, std::vector<IPAddress> &addrs, int ttl)>
      callback_t;

    protected:
      evdns_base *dns;
      evdns_request *req;
      callback_t cb;
      IPAddress source;
      SmartPointer<DNSRequest> self;

    public:
      DNSRequest(evdns_base *dns, const std::string &name,
                 DNSRequest::callback_t cb, bool search);
      DNSRequest(evdns_base *dns, uint32_t ip, DNSRequest::callback_t cb,
                 bool search);

      void cancel();
      void callback(int error, char type, int count, int ttl, void *addresses);

      static const char *getErrorStr(int error);
    };

    typedef SmartPointer<DNSRequest> DNSRequestPtr;
  }
}
