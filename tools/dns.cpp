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

#include <cbang/Catch.h>
#include <cbang/String.h>
#include <cbang/event/Base.h>
#include <cbang/dns/Base.h>
#include <cbang/config/CommandLine.h>

#include <iostream>

using namespace std;
using namespace cb;


void response(DNS::Base &dns, unsigned &count, const std::string &q,
              DNS::Error error, const vector<string> &results) {
  if (error) cerr << q << ": " << error << endl;
  else cout << q << ": " << String::join(results, ", ") << endl;
  if (!--count) dns.getEventBase().loopExit();
}


void lookup(DNS::Base &dns, unsigned &count, const std::string &q, bool ipv6,
            bool reverse) {
  if (reverse) {
    auto cb =
      [&] (DNS::Error error, const vector<string> &names) {
        response(dns, count, q, error, names);
      };

    dns.reverse(q, cb);

  } else {
    auto cb =
      [&] (DNS::Error error, const vector<SockAddr> &addrs) {
        vector<string> results;
        for (auto &addr: addrs) results.push_back(addr.toString());
        response(dns, count, q, error, results);
      };

    dns.resolve(q, cb, ipv6);
  }
}


int main(int argc, char *argv[]) {
  try {
    string server;
    bool ipv6    = false;
    bool reverse = false;

    CommandLine cmdLine;
    Logger::instance().addOptions(cmdLine);

    cmdLine.addTarget("server",  server,  "Use this nameserver", 's');
    cmdLine.addTarget("ipv6",    ipv6,    "Use IPv6", '6');
    cmdLine.addTarget("reverse", reverse, "Do a reverse lookup", 'x');

    cmdLine.parse(argc, argv);
    auto &args = cmdLine.getPositionalArgs();
    unsigned count = args.size();

    Event::Base base;
    DNS::Base dns(base, server.empty());

    if (!server.empty()) dns.addNameserver(server);

    for (auto &arg: args) lookup(dns, count, arg, ipv6, reverse);

    base.dispatch();

    return 0;
  } CATCH_ERROR;

  return 1;
}
