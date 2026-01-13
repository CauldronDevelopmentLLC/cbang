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
#include <cbang/event/Server.h>
#include <cbang/event/Buffer.h>

#include <cbang/config/CommandLine.h>
#include <cbang/log/Logger.h>
#include <cbang/os/SystemUtilities.h>

#include <signal.h>

using namespace std;
using namespace cb;


class Server : public Event::Server {
public:
  Server(Event::Base &base) : Event::Server(base) {}


  void readBuffer(const SmartPointer<Event::Connection> &conn) {
    Event::Buffer buffer;

    auto cb =
      [this, buffer, conn] (unsigned events) {
        LOG_INFO(1, String::escapeC(buffer.toString()));

        if (events &
            (Event::Connection::CLOSE_EVENT | Event::Connection::TIMEOUT_EVENT))
          return conn->close();

        readBuffer(conn);
      };

    conn->read(cb, buffer, 1024, "\n");
  }


  void writeBuffer(const SmartPointer<Event::Connection> &conn, unsigned i) {
    auto cb =
      [this, conn, i] (unsigned events) {
        if (events &
            (Event::Connection::CLOSE_EVENT | Event::Connection::TIMEOUT_EVENT))
          return;

        writeBuffer(conn, i + 1);
      };

    Event::Buffer buffer;
    buffer.add(String::printf("Hello World! %d\n", i));
    conn->write(cb, buffer);
  }


  // From Event::Server
  void onConnect(const SmartPointer<Event::Connection> &conn) {
    LOG_INFO(1, "New connection");
    readBuffer(conn);
    writeBuffer(conn, 0);
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
    Server server(base);

    server.bind(SockAddr::parse(bind), sslCtx);

    base.dispatch();

    return 0;
  } CATCH_ERROR;

  return 1;
}
