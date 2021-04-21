/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
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

#include "DNSRequest.h"

#include <cbang/net/Swab.h>
#include <cbang/Catch.h>
#include <cbang/log/Logger.h>

#include <event2/dns.h>


using namespace cb::Event;
using namespace cb;
using namespace std;


namespace {
  void dns_cb(int error, char type, int count, int ttl, void *addresses,
              void *arg) {
    ((DNSRequest *)arg)->callback(error, type, count, ttl, addresses);
  }
}


DNSRequest::DNSRequest(evdns_base *dns, const string &name,
                       DNSRequest::callback_t cb, bool search) :
  dns(dns), cb(cb), self(this) {
  req = evdns_base_resolve_ipv4
    (dns, name.c_str(), (search ? 0 : DNS_QUERY_NO_SEARCH), dns_cb, this);

  if (!req) THROW("DNS lookup failed");

  LOG_DEBUG(5, "DNS: resolving '" << name << "'");
}


DNSRequest::DNSRequest(evdns_base *dns, uint32_t ip, DNSRequest::callback_t cb,
                       bool search) : dns(dns), cb(cb), self(this) {
  struct in_addr addr = {hton32(ip)};

  req = evdns_base_resolve_reverse
    (dns, &addr, (search ? 0 : DNS_QUERY_NO_SEARCH), dns_cb, this);

  if (!req) THROW("DNS reverse lookup failed");

  LOG_DEBUG(5, "DNS: reversing " << IPAddress(ip));
}


void DNSRequest::cancel() {
  cb = 0;
  evdns_cancel_request(dns, req);
}


void DNSRequest::callback(int error, char type, int count, int ttl,
                          void *addresses) {
  LOG_DEBUG(5, "DNS: " << getErrorStr(error) << " " << (int)type
            << " " << count << " " << ttl);

  if (cb)
    try {
      // Get address results
      vector<IPAddress> addrs;
      if (error == DNS_ERR_NONE) {
        if (type == DNS_IPv4_A) {
          auto ips = (uint32_t *)addresses;

          for (int i = 0; i < count; i++)
            addrs.push_back(IPAddress(hton32(ips[i]), source.getHost()));

        } else if (type == DNS_PTR) {
          auto names = (const char **)addresses;

          for (int i = 0; i < count; i++)
            addrs.push_back(IPAddress(source.getIP(), names[i]));

        } else {
          LOG_ERROR("Unsupported DNS response type " << type);
          error = DNS_ERR_NOTIMPL;
        }
      }

      // Calback
      cb(error, addrs, ttl);

    } CATCH_ERROR;

  self.release();
}


const char *DNSRequest::getErrorStr(int error) {
  switch (error) {
  case -1:                   return "Unsupported response type";
  case DNS_ERR_NONE:         return "None";
  case DNS_ERR_CANCEL:       return "Canceled";
  case DNS_ERR_FORMAT:       return "format error";
  case DNS_ERR_NODATA:       return "No data";
  case DNS_ERR_NOTEXIST:     return "Does not exist";
  case DNS_ERR_NOTIMPL:      return "Not implemented";
  case DNS_ERR_REFUSED:      return "Refused";
  case DNS_ERR_SERVERFAILED: return "Server failed";
  case DNS_ERR_SHUTDOWN:     return "Shutdown";
  case DNS_ERR_TIMEOUT:      return "Timed out";
  case DNS_ERR_TRUNCATED:    return "Truncated";
  default:                   return "Unknown";
  }
}
