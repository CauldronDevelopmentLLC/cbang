/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include "Server.h"

#include <cbang/config.h>
#include <cbang/Catch.h>
#include <cbang/openssl/SSLContext.h>
#include <cbang/log/Logger.h>
#include <cbang/socket/Socket.h>

using namespace cb::Event;
using namespace cb;
using namespace std;


Server::Server(cb::Event::Base &base) : base(base) {}


void Server::bind(const IPAddress &addr, const SmartPointer<SSLContext> &sslCtx,
                  int priority) {
  LOG_DEBUG(4, "Binding " << (sslCtx.isSet() ? "ssl " : "") << addr);

  SmartPointer<Port> port = new Port(*this, addr, sslCtx, priority);
  port->open();
  ports.push_back(port);
}


void Server::shutdown() {
  for (auto it = ports.begin(); it != ports.end(); it++)
    (*it)->close();
}


void Server::accept(const IPAddress &peer, const SmartPointer<Socket> &socket,
                    const SmartPointer<SSLContext> &sslCtx) {
  LOG_DEBUG(4, "New connection from " << peer);

  auto conn = createConnection();

  conn->accept(peer, socket, sslCtx);
  conn->setReadTimeout(readTimeout);
  conn->setWriteTimeout(writeTimeout);
  conn->setStats(stats);
  if (maxConnectionTTL) conn->setTTL(maxConnectionTTL);

  auto cb = conn->getOnClose();
  conn->setOnClose([this, conn, cb] () mutable {
                     if (cb) cb();
                     remove(conn);
                     conn->setOnClose(0);
                   });

  connections.insert(conn);

  TRY_CATCH_ERROR(onConnect(conn));
}


void Server::remove(const SmartPointer<Connection> &conn) {
  LOG_DEBUG(4, "Connection ended");

  connections.erase(conn);

  for (auto it = ports.begin(); it != ports.end(); it++)
    (*it)->activate();
}


SmartPointer<Connection> Server::createConnection() {
  return new Connection(base);
}
