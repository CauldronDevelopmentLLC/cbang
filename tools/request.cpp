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

#include <cbang/event/Base.h>
#include <cbang/dns/Base.h>
#include <cbang/http/Client.h>
#include <cbang/http/OutgoingRequest.h>

#include <cbang/config/CommandLine.h>
#include <cbang/openssl/SSLContext.h>

using namespace std;
using namespace cb;


int main(int argc, char *argv[]) {
  try {
    string method = "GET";
    string _url;

    CommandLine cmdLine;
    Logger::instance().addOptions(cmdLine);

    cmdLine.addTarget("method", method, "HTTP request method", 'x');
    cmdLine.addTarget("url", _url, "HTTP URL", 'u');

    cmdLine.parse(argc, argv);

    URI url(_url);
    SmartPointer<SSLContext> sslCtx;
    if (url.getScheme() == "https") sslCtx = new SSLContext;

    Event::Base base;
    DNS::Base dns(base);
    HTTP::Client client(base, dns, sslCtx);

    auto cb =
      [&] (HTTP::Request &req) {
        if (req.getConnectionError()) LOG_ERROR(req.getConnectionError());
        else {
          LOG_INFO(1, req.getInputHeaders() << '\n');
          LOG_INFO(1, req.getInput());
        }

        base.loopExit();
      };

    auto req = client.call(url, HTTP::Method::parse(method), cb);
    req->send();

    base.dispatch();

    return 0;
  } CATCH_ERROR;

  return 1;
}
