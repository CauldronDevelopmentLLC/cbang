/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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
#include "DNSBase.h"

#include <cbang/Catch.h>
#include <cbang/socket/Socket.h>
#include <cbang/log/Logger.h>

using namespace cb::Event;
using namespace cb;
using namespace std;

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX << "CON" << getID() << ':'


uint64_t Connection::nextID = 0;


Connection::Connection(cb::Event::Base &base) :
  FD(base),
  timeout(base.newEvent(this, &Connection::timedout, EVENT_NO_SELF_REF)) {
  LOG_DEBUG(4, "Connection opened");
}


Connection::~Connection() {
  LOG_DEBUG(4, "Connection closed");
  if (socket.isSet()) socket->adopt(); // Prevent socket from closing FD
}


void Connection::setTTL(double sec) {timeout->add(sec);}


bool Connection::isConnected() const {
  return getFD() != -1 && socket.isSet() && socket->isConnected();
}


void Connection::accept(const IPAddress &peer,
                        const SmartPointer<Socket> &socket,
                        const SmartPointer<SSL> &ssl) {
  if (socket.isNull()) THROW("Socket cannot be null");
  setPeer(peer);
  setSSL(ssl); // Must be before setFD()
  this->socket = socket;
  setFD(socket->get());

  LOG_DEBUG(4, "Connection accepted with fd " << socket->get());
}


void Connection::connect(DNSBase &dns, const IPAddress &peer,
                         const IPAddress &bind, function<void (bool)> cb) {
  try {
    // Open and bind new socket
    socket = new Socket;
    if (bind.getIP()) socket->bind(bind);
    else socket->open();
    socket->setBlocking(false);
    setFD(socket->get());
    setPeer(peer);

    LOG_DEBUG(4, "Connection connecting with fd " << socket->get());

#ifdef HAVE_OPENSSL
    // SSL
    if (getSSL().isSet()) {
      getSSL()->setFD(socket->get());
      getSSL()->setConnectState();
      if (peer.hasHost()) getSSL()->setTLSExtHostname(peer.getHost());
    }
#endif // HAVE_OPENSSL

    // DNS request reference
    struct Ref {SmartPointer<DNSRequest> dnsReq;};
    SmartPointer<Ref> ref = new Ref;

    // DNS callback
    auto dnsCB =
      [this, ref, peer, cb] (int err, vector<IPAddress> &addrs, int ttl) {
        ref->dnsReq.release();

        if (err || addrs.empty()) {
          LOG_WARNING("DNS lookup failed for " << peer);
          if (cb) cb(false);
          return;
        }

        try {
          // Use first address
          IPAddress addr(addrs[0].getIP(), peer.getHost(), peer.getPort());
          setPeer(addr);

          // NOTE, Connect timeout is write timeout
          socket->connect(addr);

          // Wait for socket write
          auto writeCB = [this, cb] (bool success) {if (cb) cb(isConnected());};
          canWrite(writeCB);
          return;

        } CATCH_WARNING;

        if (cb) cb(false);
      };

    // Skip DNS lookup if we already have an IP
    if (peer.getIP()) {
      vector<IPAddress> addrs;
      addrs.push_back(peer);
      return dnsCB(0, addrs, 0);
    }

    // Start async DNS lookup
    ref->dnsReq = dns.resolve(peer.getHost(), dnsCB);
    return;

  } CATCH_ERROR;

  if (cb) cb(false);
}


void Connection::timedout() {
  if (stats.isSet()) stats->event("timedout");
  LOG_DEBUG(3, "Connection timedout");
  close();
}
