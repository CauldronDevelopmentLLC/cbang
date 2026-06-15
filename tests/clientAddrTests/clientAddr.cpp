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

// Test driver for HTTP::Request::resolveClientAddr().  Reads a JSON document
// on stdin and prints the resolved client address (without port):
//
//   {"trusted": "<CIDR spec>", "peer": "<addr>",
//    "xff": "<X-Forwarded-For>", "real-ip": "<X-Real-IP>"}

#include <cbang/Catch.h>
#include <cbang/json/Reader.h>
#include <cbang/http/Request.h>
#include <cbang/net/AddressRangeSet.h>
#include <cbang/net/SockAddr.h>
#include <cbang/log/Logger.h>

#include <iostream>

using namespace cb;
using namespace std;


int main() {
  try {
    Logger::instance().setLogTime(false);
    Logger::instance().setLogColor(false);
    Exception::printLocations    = false;
    Exception::enableStackTraces = false;

    auto input = JSON::Reader::parse(cin);

    AddressRangeSet trusted;
    trusted.insert(input->getString("trusted", ""));

    SockAddr peer = SockAddr::parse(input->getString("peer"));
    string xff    = input->getString("xff",     "");
    string realIP = input->getString("real-ip", "");

    cout << HTTP::Request::resolveClientAddr(peer, trusted, xff, realIP)
      .toString(false) << endl;

    return 0;
  } CATCH_ERROR;

  return 1;
}
