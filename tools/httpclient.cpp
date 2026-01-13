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

#include <cbang/Catch.h>
#include <cbang/String.h>

#include <cbang/event/Base.h>
#include <cbang/dns/Base.h>
#include <cbang/http/Client.h>

#include <cbang/config/CommandLine.h>
#include <cbang/openssl/SSLContext.h>
#include <cbang/log/Logger.h>

#include <signal.h>

using namespace std;
using namespace cb;


int main(int argc, char *argv[]) {
  try {
    string bind      = "";
    string certFile;
    string priFile;
    string url       = "";
    unsigned timeout = 50;

    CommandLine cmdLine;
    Logger::instance().addOptions(cmdLine);
    Logger::instance().setLogHeader(false);

    cmdLine.addTarget("bind", bind, "Outgoing IP address", 'b');
    cmdLine.addTarget("url", url, "Request URl", 'u');
    cmdLine.addTarget("timeout", timeout, "Set read/write timeout in seconds");
    cmdLine.add("method", "Request method")->setDefault("GET");
    cmdLine.add("header", 'H', 0, "Add HTTP header <name>=<value>")->
      setType(Option::TYPE_STRINGS);
    cmdLine.parse(argc, argv);

    if (url.empty()) {
      cout << "URL not set" << endl;
      return 1;
    }

    SmartPointer<SSLContext> sslCtx;
    if (String::toLower(URI(url).getScheme()) == "https") {
      sslCtx = new SSLContext;
      sslCtx->loadSystemRootCerts();
    }

    auto method = HTTP::Method::parse(cmdLine["--method"]);

    ::signal(SIGPIPE, SIG_IGN);

    Event::Base base;
    HTTP::Client client(base, sslCtx);

    client.setReadTimeout(timeout);
    client.setWriteTimeout(timeout);

    auto cb =
      [&base] (HTTP::Request &req) {
        LOG_INFO(1, req.getInputHeaders() << '\n' << req.getInput());
        base.loopExit();
      };

    auto req = client.call(url, method, cb);

    auto &outHdrs = req->getRequest()->getOutputHeaders();
    auto hdrs = cmdLine["header"].toStrings();
    for (auto hdr: hdrs) {
      auto eq = hdr.find_first_of('=');
      if (eq != string::npos)
        outHdrs.set(hdr.substr(0, eq), hdr.substr(eq + 1));
      else outHdrs.set(hdr, "");
    }

    req->send();

    base.dispatch();

    return 0;
  } CATCH_ERROR;

  return 1;
}
