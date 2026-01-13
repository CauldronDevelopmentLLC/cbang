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

#include "RequestReverse.h"

#include <cbang/String.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::DNS;


namespace {
  char nibble(uint8_t x) {return "0123456789abcdef"[x & 0xf];}
}


namespace {
  string addrToRequest(const SockAddr &addr) {
    if (addr.isIPv4()) {
      uint32_t ip = addr.getIPv4();

      return String::printf(
        "%d.%d.%d.%d.in-addr.arpa",
        (int)((ip >>  0) & 0xff), (int)((ip >>  8) & 0xff),
        (int)((ip >> 16) & 0xff), (int)((ip >> 24) & 0xff));

    } else if (addr.isIPv6()) {
      char buf[73] = "0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0."
        "0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.ip6.arpa";

      auto a = addr.getIPv6();
      for (int i = 0; i < 16; i++) {
        uint8_t byte = a[15 - i];
        buf[i * 4 + 0] = nibble(byte >> 0);
        buf[i * 4 + 2] = nibble(byte >> 4);
      }

      return buf;

    } else THROW("Unsupported address type");
  }
}


RequestReverse::RequestReverse(
  Base &base, const SockAddr &addr, callback_t cb) :
  Request(base, addrToRequest(addr)), cb(cb) {
  LOG_DEBUG(5, "DNS: reversing " << addr);
}


void RequestReverse::callback() {
  LOG_DEBUG(5, "DNS: reverse response: " << result->error);
  if (cb) cb(result->error, result->names);
}
