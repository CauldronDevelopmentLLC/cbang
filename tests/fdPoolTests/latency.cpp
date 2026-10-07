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

// Request latency through the FD pool.  An HTTP::Server and an HTTP::Client
// in this program, on 127.0.0.1, make requests one after another.  Each
// request queues several FD pool commands, and a quiet pool thread waits in
// epoll for up to 100ms, so unless queueing a command wakes it, every request
// waits out that timeout several times.
//
//   latency [count]

#include <cbang/Catch.h>
#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/event/Base.h>
#include <cbang/http/Client.h>
#include <cbang/http/Request.h>
#include <cbang/http/RequestHandler.h>
#include <cbang/http/Server.h>
#include <cbang/log/Logger.h>
#include <cbang/net/URI.h>
#include <cbang/time/Timer.h>

#include <iostream>
#include <vector>

using namespace cb;
using namespace std;


int main(int argc, char *argv[]) {
  try {
    Logger::instance().setScreenStream(cerr);
    Logger::instance().setVerbosity(0);

    unsigned count = 1 < argc ? String::parseU32(argv[1]) : 20;

    Event::Base base;

    HTTP::Server server(base);
    server.addHandler(new HTTP::RequestFunctionHandler(
      [] (HTTP::Request &req) {req.reply("ok", 2); return true;}));

    // A free local port
    SockAddr addr;
    for (unsigned port = 18723; ; port++) {
      if (port == 19223) THROW("No free port");
      addr = SockAddr((uint32_t)0x7f000001, (uint16_t)port);

      try {
        server.addListenPort(addr);
        break;
      } catch (const Exception &e) {}
    }

    HTTP::Client client(base);
    URI uri("http", "127.0.0.1", addr.getPort());

    // Each request is sent when the one before it is answered.  The requests
    // are held until the end, since letting one go cancels it.
    vector<HTTP::Client::RequestPtr> requests;
    unsigned done = 0, ok = 0;

    function<void ()> next = [&] () {
      auto pr = client.call(uri, HTTP::Method::HTTP_GET,
        [&] (HTTP::Request &req) {
          if (req.getResponseCode() == HTTP::Status::HTTP_OK &&
              req.getInput() == "ok") ok++;

          if (++done == count) base.loopExit();
          else next();
        });

      requests.push_back(pr);
      pr->send();
    };

    // Guard against a request that never completes
    auto guard = base.newEvent([&] () {base.loopExit();});
    guard->add(60);

    double start = Timer::now();
    next();
    base.dispatch();
    double elapsed = Timer::now() - start;

    cout << ok << " of " << count << " requests answered\n";

    // 20 requests took over 4s while each command waited for the timeout
    cout << "under 1s: " << (elapsed < 1 ? "yes" : "no") << endl;
    if (1 <= elapsed) cerr << "Took " << elapsed << "s" << endl;

    return 0;
  } CATCH_ERROR;

  return 1;
}
