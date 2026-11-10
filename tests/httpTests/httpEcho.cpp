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

// An HTTP echo server for httpTests.  httptest.py drives it with hand-built
// requests.  Each request is answered with "<method> <path> body=<body>", so
// a test sees exactly where the server thinks one request ends and the next
// begins.  GET /quit exits.
//
//   httpEcho <port>

#include <cbang/Catch.h>
#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/event/Base.h>
#include <cbang/http/Request.h>
#include <cbang/http/RequestHandler.h>
#include <cbang/http/Server.h>
#include <cbang/log/Logger.h>

#include <iostream>

using namespace std;
using namespace cb;


int main(int argc, char *argv[]) {
  try {
    Logger::instance().setScreenStream(cerr);
    Logger::instance().setVerbosity(0);
    Exception::printLocations = false;

    if (argc != 2) {
      cerr << "Usage: " << argv[0] << " <port>" << endl;
      return 1;
    }

    uint16_t port = String::parseU16(argv[1]);
    Event::Base base;
    HTTP::Server server(base);

    server.addHandler(new HTTP::RequestFunctionHandler(
      [&] (HTTP::Request &req) {
        string path = req.getURI().getPath();
        if (path == "/quit") base.loopExit();

        string body = req.getInputBuffer().toString();
        req.send(SSTR(req.getMethod() << ' ' << path << " body=\""
                      << String::escapeC(body) << '"'));
        req.reply();
        return true;
      }));

    server.addListenPort(SockAddr((uint32_t)0x7f000001, port));

    bool timedOut = false;
    auto guard = base.newEvent([&] () {timedOut = true; base.loopExit();});
    guard->add(30);

    cout << "server: listening" << endl;
    base.dispatch();

    if (timedOut) {
      cout << "server: timed out" << endl;
      return 1;
    }

    return 0;
  } CATCH_ERROR;

  return 1;
}
