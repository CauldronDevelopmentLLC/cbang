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

// A Websocket echo server and client for wsTests.  wstest.py drives the
// server with hand-built frames; client mode exercises Websocket::connect().
// Events are logged to stdout, one per line, without IDs, ports or times.
//
//   wsEcho server <port> [--max-size N] [--close-on-message] [--connections N]
//   wsEcho client <port> [message]

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
#include <cbang/ws/Websocket.h>

#include <iostream>
#include <vector>

using namespace cb;
using namespace std;


namespace {
  string describe(const char *data, uint64_t length) {
    string s(data, length);
    if (length <= 64) return "\"" + String::escapeC(s) + "\"";

    // FNV-1a, so large payloads can be compared without dumping them
    uint32_t hash = 2166136261u;
    for (uint64_t i = 0; i < length; i++)
      hash = (hash ^ (uint8_t)data[i]) * 16777619u;

    return SSTR("fnv1a=" << String::printf("%08x", hash));
  }


  class EchoWS : public WS::Websocket {
    const string name;
    function<void ()> done;
    bool closeOnMessage = false;

  public:
    EchoWS(const string &name, function<void ()> done) :
      name(name), done(done) {}

    void setCloseOnMessage(bool x) {closeOnMessage = x;}

    void log(const string &msg) {cout << name << ": " << msg << endl;}

    // From WS::Websocket
    void onOpen() override {log("onOpen");}


    void onMessage(const char *data, uint64_t length) override {
      log(SSTR("onMessage len=" << length << ' ' << describe(data, length)));

      if (closeOnMessage) {
        close(WS::Status::WS_STATUS_NORMAL, "bye");

        try {
          send("after close");
          log("send after close: accepted");
        } catch (const Exception &e) {
          log("send after close: refused: " + e.getMessage());
        }

      } else send(data, length);
    }


    void onClose(WS::Status status, const string &msg) override {
      log(SSTR("onClose " << (unsigned)status << " \"" << String::escapeC(msg)
               << '"'));
    }


    void onShutdown() override {
      log("onShutdown");
      done();
    }
  };


  int server(Event::Base &base, uint16_t port, int argc, char *argv[]) {
    unsigned maxSize     = 0;
    bool closeOnMessage  = false;
    unsigned connections = 1;

    for (int i = 0; i < argc; i++) {
      string arg = argv[i];
      if (arg == "--max-size" && i + 1 < argc)
        maxSize = String::parseU32(argv[++i]);
      else if (arg == "--close-on-message") closeOnMessage = true;
      else if (arg == "--connections" && i + 1 < argc)
        connections = String::parseU32(argv[++i]);
      else THROW("Invalid argument: " << arg);
    }

    // Exit once the expected number of Websockets have shut down
    unsigned shutdowns = 0;
    auto done = [&] () {if (++shutdowns == connections) base.loopExit();};

    // Websockets are held until exit so callbacks never outlive them
    vector<SmartPointer<EchoWS>> websockets;

    HTTP::Server server(base);
    server.addHandler(new HTTP::RequestFunctionHandler(
      [&] (HTTP::Request &req) {
        auto ws = SmartPtr(new EchoWS("server", done));
        if (maxSize) ws->setMaxMessageSize(maxSize);
        ws->setCloseOnMessage(closeOnMessage);
        websockets.push_back(ws);
        ws->upgrade(req);
        return true;
      }));

    server.addListenPort(SockAddr((uint32_t)0x7f000001, port));

    bool timedOut = false;
    auto guard = base.newEvent([&] () {timedOut = true; base.loopExit();});
    guard->add(30);

    cout << "server: listening" << endl;
    base.dispatch();

    if (timedOut) {
      cout << "server: timed out after " << shutdowns << " of " << connections
           << " shutdowns" << endl;
      return 1;
    }

    return 0;
  }


  class ClientWS : public EchoWS {
  public:
    using EchoWS::EchoWS;

    // From WS::Websocket
    void onOpen() override {
      EchoWS::onOpen();
      send(message);
    }


    void onMessage(const char *data, uint64_t length) override {
      log(SSTR("onMessage len=" << length << ' ' << describe(data, length)));
      close(WS::Status::WS_STATUS_NORMAL, "done");
    }

    string message;
  };


  int client(Event::Base &base, uint16_t port, int argc, char *argv[]) {
    bool shutdown = false;
    auto done = [&] () {shutdown = true; base.loopExit();};

    auto ws = SmartPtr(new ClientWS("client", done));
    ws->message = argc ? argv[0] : "hello";

    HTTP::Client client(base);
    auto conn = ws->connect(client, URI("ws", "127.0.0.1", port, "/"));

    auto guard = base.newEvent([&] () {base.loopExit();});
    guard->add(30);

    base.dispatch();

    if (!shutdown) {
      cout << "client: timed out" << endl;
      return 1;
    }

    return 0;
  }
}


int main(int argc, char *argv[]) {
  try {
    Logger::instance().setScreenStream(cerr);
    Logger::instance().setVerbosity(0);
    Exception::printLocations = false;

    if (argc < 3) {
      cerr << "Usage: " << argv[0] << " server|client <port> [options]"
           << endl;
      return 1;
    }

    string mode   = argv[1];
    uint16_t port = String::parseU16(argv[2]);

    Event::Base base;

    if (mode == "server") return server(base, port, argc - 3, argv + 3);
    if (mode == "client") return client(base, port, argc - 3, argv + 3);
    THROW("Invalid mode: " << mode);

  } CATCH_ERROR;

  return 1;
}
