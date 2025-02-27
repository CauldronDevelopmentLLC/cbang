/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "Connection.h"
#include "Event.h"
#include "Server.h"

#include <cbang/Catch.h>
#include <cbang/net/Socket.h>
#include <cbang/log/Logger.h>
#include <cbang/dns/Base.h>
#include <cbang/util/WeakCallback.h>

using namespace cb::Event;
using namespace cb;
using namespace std;

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX "CON" << getID() << ':'


uint64_t Connection::nextID = 0;


Connection::Connection(Base &base) : FD(base),
  timeout(base.newEvent(this, &Connection::timedout)) {
  LOG_DEBUG(4, "Connection opened");
}


Connection::~Connection() {LOG_DEBUG(4, "Connection closed");}


Server &Connection::getServer() const {
  if (!server) THROW("Server not set");
  return *server;
}


void Connection::setTTL(double sec) {
  if (sec <= 0) timeout->del();
  else timeout->add(sec);
}


void Connection::setSocket(const SmartPointer<Socket> &socket) {
  this->socket = socket;
  if (socket.isSet()) socket->setAutoClose(false);
  setFD(socket.isSet() ? socket->get() : -1);
#ifdef HAVE_OPENSSL
  if (getFD() != -1 && getSSL().isSet()) getSSL()->setFD(getFD());
#endif
}


bool Connection::isConnected() const {return getFD() != -1;}


void Connection::accept(const SockAddr &peerAddr,
                        const SmartPointer<Socket> &socket,
                        const SmartPointer<SSLContext> &sslCtx) {
  if (isConnected()) THROW("Already connected");

  LOG_DEBUG(5, "Accepting from " << peerAddr);
  if (stats.isSet()) stats->event("incoming");

  socket->setBlocking(false);

  SmartPointer<SSL> ssl;
#ifdef HAVE_OPENSSL
  if (sslCtx.isSet()) {
    ssl = sslCtx->createSSL();
    ssl->setFD(socket->get());
    ssl->accept();
  }
#endif // HAVE_OPENSSL

  setSSL(ssl);
  this->peerAddr = peerAddr;
  setSocket(socket);

  LOG_DEBUG(4, "Connection accepted with fd " << socket->get());
}


void Connection::openSSL(SSLContext &sslCtx, const string &hostname) {
#ifdef HAVE_OPENSSL
  auto ssl = sslCtx.createSSL();
  if (getFD() != -1) ssl->setFD(getFD());
  ssl->setConnectState();
  ssl->setTLSExtHostname(hostname);
  setSSL(ssl);
#endif // HAVE_OPENSSL
}


void Connection::connect(
  const string &hostname, uint32_t port, const SockAddr &bind) {
  try {
    if (isConnected()) THROW("Already connected");

    LOG_DEBUG(5, "Connecting to " << hostname << ":" << port);
    if (stats.isSet()) stats->event("outgoing");

    // Open and bind new socket
    auto socket = SmartPtr(new Socket);
    socket->open(Socket::NONBLOCKING, bind);
    setSocket(socket);

    LOG_DEBUG(4, "Connection with fd " << socket->get());

    // DNS callback
    DNS::RequestResolve::callback_t dnsCB =
      [this, hostname, port] (
        DNS::Error error, const vector<SockAddr> &addrs) {

        if (error || addrs.empty()) {
          LOG_WARNING("DNS lookup failed for " << hostname);
          return onConnect(false);
        }

        try {
          // Set address
          peerAddr = addrs.front();
          peerAddr.setPort(port);
          LOG_DEBUG(4, "Connecting to " << peerAddr);

          // NOTE, Connect timeout is write timeout
          getSocket()->connect(peerAddr);

          // Wait for socket write
          Transfer::cb_t writeCB = [this] (bool success) {
            onConnect(success && isConnected());
          };
          canWrite(WeakCall(this, writeCB));
          return;

        } CATCH_WARNING;

        close();
        onConnect(false);
      };

    // Start async DNS lookup
    getBase().getDNS().resolve(hostname, WeakCall(this, dnsCB));
    return;

  } CATCH_ERROR;

  close();

  onConnect(false);
}


void Connection::close() {
  auto self = SmartPtr(this);
  if (server) server->remove(this);
  FD::close();
  setSSL(0);
  setSocket(0);
}


void Connection::timedout() {
  if (stats.isSet()) stats->event("timedout");
  LOG_DEBUG(3, "Connection timedout");
  close();
}
