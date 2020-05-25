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

#include <cbang/Catch.h>
#include <cbang/String.h>

#include <cbang/event/Base.h>
#include <cbang/event/DNSBase.h>
#include <cbang/event/Client.h>

#include <cbang/config/CommandLine.h>
#include <cbang/openssl/SSLContext.h>
#include <cbang/log/Logger.h>

#include <signal.h>

using namespace std;
using namespace cb;


int main(int argc, char *argv[]) {
  try {
    string bind = "";
    string certFile;
    string priFile;
    string url = "http://www.google.com/";

    CommandLine cmdLine;
    Logger::instance().addOptions(cmdLine);
    Logger::instance().setLogHeader(false);

    cmdLine.addTarget("bind", bind, "Outgoing IP address", 'b');
    cmdLine.addTarget("url", url, "Request URl", 'u');
    cmdLine.add("method", "Request method")->setDefault("GET");
    cmdLine.parse(argc, argv);

    SmartPointer<SSLContext> sslCtx;
    if (String::toLower(URI(url).getScheme()) == "https")
      sslCtx = new SSLContext;

    Event::RequestMethod method =
      Event::RequestMethod::parse(cmdLine["--method"]);

    ::signal(SIGPIPE, SIG_IGN);

    Event::Base base(true);
    Event::DNSBase dnsBase(base);
    Event::Client client(base, dnsBase, sslCtx);

    auto cb =
      [&base] (Event::Request &req) {
        LOG_INFO(1, req.getInputHeaders() << '\n' << req.getInput());
        base.loopExit();
      };

    auto req = client.call(url, method, cb);
    req->send();

    base.dispatch();

    return 0;
  } CATCH_ERROR;

  return 1;
}
