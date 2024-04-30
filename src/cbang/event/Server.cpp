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

#include "Server.h"

#include <cbang/config.h>
#include <cbang/Catch.h>
#include <cbang/openssl/SSLContext.h>
#include <cbang/log/Logger.h>
#include <cbang/net/Socket.h>
#include <cbang/config/Options.h>

using namespace cb::Event;
using namespace cb;
using namespace std;


Server::Server(Base &base) : base(base), addrFilter(&base.getDNS()) {}


void Server::setTimeout(int timeout) {
  setReadTimeout(timeout);
  setWriteTimeout(timeout);
}


void Server::allow(const string &spec) {addrFilter.allow(spec);}
void Server::deny (const string &spec) {addrFilter.deny(spec);}


void Server::addOptions(Options &options) {
  options.pushCategory("Server");

  options.add("allow", "Client addresses which are allowed to connect to this "
              "server.  This option overrides addresses which are denied in "
              "the deny option.  The pattern 0/0 matches all addresses."
              )->setDefault("0/0");
  options.add("deny", "Client address which are not allowed to connect to this "
              "server.")->setDefault("");

  options.add("connection-timeout", "Maximum time in seconds before a client "
              "connection times out.  Zero indicates no timeout.");
  options.addTarget("connection-backlog", connectionBacklog,
                    "Size of the connection backlog queue.  Once this is full "
                    "connections are rejected.");
  options.addTarget("max-connections", maxConnections,
                    "Maximum simultaneous client connections per port");
  options.addTarget("max-ttl", maxConnectionTTL,
                    "Maximum client connection time in seconds");

  options.popCategory();
}


void Server::init(Options &options) {
  allow(options["allow"]);
  deny(options["deny"]);

  if (options["connection-timeout"].hasValue())
    setTimeout(options["connection-timeout"].toInteger());
}


void Server::bind(const SockAddr &addr, const SmartPointer<SSLContext> &sslCtx,
                  int priority) {
  LOG_DEBUG(4, "Binding " << (sslCtx.isSet() ? "ssl " : "") << addr);

  SmartPointer<Port> port = new Port(*this, addr, sslCtx, priority);
  port->open();
  ports.push_back(port);
}


void Server::shutdown() {for (auto &port: ports) port->close();}


void Server::accept(const SockAddr &peerAddr,
                    const SmartPointer<Socket> &socket,
                    const SmartPointer<SSLContext> &sslCtx) {
  if (!isAllowed(peerAddr)) {
    LOG_DEBUG(3, "Denying connection from " << peerAddr);
    return;
  }

  LOG_DEBUG(4, "New connection from " << peerAddr);

  auto conn = createConnection();

  conn->accept(peerAddr, socket, sslCtx);
  conn->setReadTimeout(readTimeout);
  conn->setWriteTimeout(writeTimeout);
  conn->setStats(stats);
  if (maxConnectionTTL) conn->setTTL(maxConnectionTTL);

  conn->setServer(this);
  connections.insert(conn);

  TRY_CATCH_ERROR(conn->onConnect());
}


void Server::remove(const SmartPointer<Connection> &conn) {
  LOG_DEBUG(4, "Connection ended");

  connections.erase(conn);

  for (auto &port: ports) port->activate();
}


bool Server::isAllowed(const SockAddr &peerAddr) const {
  return addrFilter.isAllowed(peerAddr);
}


SmartPointer<Connection> Server::createConnection() {
  return new Connection(base);
}
