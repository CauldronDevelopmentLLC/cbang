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
#include <cbang/http/Server.h>
#include <cbang/http/Request.h>

#include <cbang/config/CommandLine.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/log/Logger.h>

#include <signal.h>

using namespace std;
using namespace cb;


class HTTPServer : public HTTP::Server {
public:
  HTTPServer(Event::Base &base) : HTTP::Server(base) {}

  // From HTTP::Server
  bool operator()(HTTP::Request &req) override {
    string path = req.getURI().getPath();

    if (path == "/") req.reply("<h1>Hello World!</h1>");
    else if (path == "/err") THROW("Error");
    else if (path == "/bad") THROWX("Bad request", HTTP_BAD_REQUEST);

    else if (path == "/chunked") {
      req.startChunked();
      for (unsigned i = 0; i < 10; i++)
        req.sendChunk(String::printf("<h2>%d</h2>", i));
      req.endChunked();

    } else return false;

    return true;
  }
};


int main(int argc, char *argv[]) {
  try {
    string bind = "0.0.0.0:8000";
    string certFile;
    string priFile;

    CommandLine cmdLine;
    Logger::instance().addOptions(cmdLine);

    cmdLine.addTarget("bind", bind, "Server listen address and port", 'b');
    cmdLine.addTarget("cert", certFile, "Certificate chain file", 'c');
    cmdLine.addTarget("private", priFile, "Private key file", 'p');
    cmdLine.parse(argc, argv);

    SmartPointer<SSLContext> sslCtx;

    if (!certFile.empty() && !priFile.empty()) {
      sslCtx = new SSLContext;

      LOG_INFO(1, "Loading certificate");
      sslCtx->useCertificateChainFile(certFile);

      LOG_INFO(1, "Loading private key");
      sslCtx->usePrivateKey(*SystemUtilities::open(priFile));
    }

    ::signal(SIGPIPE, SIG_IGN);

    Event::Base base(true);
    HTTPServer server(base);

    server.bind(SockAddr::parse(bind), sslCtx);

    base.dispatch();

    return 0;
  } CATCH_ERROR;

  return 1;
}
