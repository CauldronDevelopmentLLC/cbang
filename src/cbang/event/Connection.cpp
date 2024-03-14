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

#include "Connection.h"
#include "Event.h"
#include "Server.h"

#include <cbang/Catch.h>
#include <cbang/net/Socket.h>
#include <cbang/log/Logger.h>
#include <cbang/dns/Base.h>

using namespace cb::Event;
using namespace cb;
using namespace std;

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX "CON" << getID() << ':'


uint64_t Connection::nextID = 0;


Connection::Connection(Base &base) : FD(base),
  timeout(base.newEvent(this, &Connection::timedout, EVENT_NO_SELF_REF)) {
  LOG_DEBUG(4, "Connection opened");
}


Connection::~Connection() {
  LOG_DEBUG(4, "Connection closed");
  // Prevent socket from closing FD
  if (socket.isSet()) TRY_CATCH_ERROR(socket->adopt());
}


void Connection::setTTL(double sec) {
  if (sec <= 0) timeout->del();
  else timeout->add(sec);
}


void Connection::setSocket(const SmartPointer<Socket> &socket) {
  // Prevent socket from closing FD
  if (this->socket.isSet()) this->socket->adopt();
  this->socket = socket;

  setFD(socket.isSet() ? socket->get() : -1);
}


bool Connection::isConnected() const {
  return getFD() != -1 && socket.isSet() && socket->isOpen();
}


void Connection::accept(const SockAddr &peerAddr,
                        const SmartPointer<Socket> &socket,
                        const SmartPointer<SSLContext> &sslCtx) {
  if (socket.isNull()) THROW("Socket cannot be null");

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


void Connection::connect(
  DNS::Base &dns, const string &hostname, uint32_t port, const SockAddr &bind,
  const SmartPointer<SSLContext> &sslCtx, function<void (bool)> cb) {
  try {
    if (isConnected()) THROW("Already connected");

    LOG_DEBUG(5, "Connecting to " << hostname << ":" << port);
    if (stats.isSet()) stats->event("outgoing");

    // Open and bind new socket
    SmartPointer<Socket> socket = new Socket;
    socket->open();
    if (!bind.isNull()) socket->bind(bind);
    socket->setBlocking(false);

    SmartPointer<SSL> ssl;
#ifdef HAVE_OPENSSL
    if (sslCtx.isSet()) {
      ssl = sslCtx->createSSL();
      ssl->setFD(socket->get());
      ssl->setConnectState();
      ssl->setTLSExtHostname(hostname);
    }
#endif // HAVE_OPENSSL

    setSSL(ssl);
    setSocket(socket);

    LOG_DEBUG(4, "Connection with fd " << socket->get());

    // DNS request reference
    struct Ref {SmartPointer<DNS::Request> dnsReq;};
    SmartPointer<Ref> ref = new Ref;

    // DNS callback
    auto dnsCB =
      [this, ref, hostname, port, cb] (
        DNS::Error error, const vector<SockAddr> &addrs) {
        ref->dnsReq.release();

        if (error || addrs.empty()) {
          LOG_WARNING("DNS lookup failed for " << hostname);
          if (cb) cb(false);
          return;
        }

        try {
          // Set address
          peerAddr = addrs.front();
          peerAddr.setPort(port);
          LOG_DEBUG(4, "Connecting to " << peerAddr);

          // NOTE, Connect timeout is write timeout
          getSocket()->connect(peerAddr);

          // Wait for socket write
          auto writeCB = [this, cb] (bool success) {
            if (cb) cb(success && isConnected());
          };
          canWrite(writeCB);
          return;

        } CATCH_WARNING;

        if (cb) cb(false);
      };

    // Start async DNS lookup
    ref->dnsReq = dns.resolve(hostname, dnsCB);
    return;

  } CATCH_ERROR;

  if (cb) cb(false);
}


void Connection::timedout() {
  if (stats.isSet()) stats->event("timedout");
  LOG_DEBUG(3, "Connection timedout");
  close();
}
