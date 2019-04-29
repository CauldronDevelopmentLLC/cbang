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

#include "HTTP.h"
#include "Base.h"
#include "Request.h"
#include "HTTPStatus.h"
#include "Headers.h"
#include "Event.h"
#include "Buffer.h"
#include "BufferEvent.h"
#include "Connection.h"

#include <cbang/Exception.h>
#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/time/Timer.h>
#include <cbang/socket/Socket.h>

#ifdef HAVE_OPENSSL
#include <cbang/openssl/SSLContext.h>
#else
namespace cb {class SSLContext {};}
#endif

using namespace std;
using namespace cb::Event;


HTTP::HTTP(Base &base, const SmartPointer<HTTPHandler> &handler,
           const cb::SmartPointer<cb::SSLContext> &sslCtx) :
  base(base), handler(handler), sslCtx(sslCtx) {

#ifndef HAVE_OPENSSL
  if (!sslCtx.isNull()) THROW("C! was not built with openssl support");
#endif
}


HTTP::~HTTP() {}


void HTTP::setMaxConnectionTTL(unsigned x) {
  maxConnectionTTL = x;

  if (maxConnectionTTL) {
    if (expireEvent.isNull())
      expireEvent = base.newEvent(this, &HTTP::expireCB, true);

    expireEvent->add(60); // Check once per minute

  } else if (expireEvent->isPending()) expireEvent->del();
}


void HTTP::remove(Connection &con) {connections.remove(&con);}


void HTTP::bind(const cb::IPAddress &addr) {
  // TODO Support binding multiple addresses
  if (this->socket.isSet()) THROW("Already bound");

  SmartPointer<Socket> socket = new Socket;
  socket->setReuseAddr(true);
  socket->bind(addr);
  socket->listen(128);
  socket_t fd = socket->get();

  // TODO This event should be destroyed with the HTTP
  base.newEvent(fd, EVENT_READ | EVENT_PERSIST, this, &HTTP::acceptCB)->add();

  this->socket = socket;
  boundAddr = addr;
}


cb::SmartPointer<Request> HTTP::createRequest
(RequestMethod method, const URI &uri, const Version &version) {
  return handler->createRequest(method, uri, version);
}


void HTTP::handleRequest(Request &req) {
  LOG_DEBUG(5, "New request on " << boundAddr << ", connection count = "
            << getConnectionCount());

  TRY_CATCH_ERROR(dispatch(*handler, req));
}


bool HTTP::dispatch(HTTPHandler &handler, Request &req) {
  try {
    if (handler.handleRequest(req)) {
      handler.endRequest(req);
      return true;
    }

    req.sendError(HTTPStatus::HTTP_NOT_FOUND);

  } catch (cb::Exception &e) {
    if (!CBANG_LOG_DEBUG_ENABLED(3)) LOG_WARNING(e.getMessage());
    LOG_DEBUG(3, e);
    req.sendError(e);

  } catch (std::exception &e) {
    LOG_ERROR(e.what());
    req.sendError(e);

  } catch (...) {
    LOG_ERROR(HTTPStatus(HTTPStatus::HTTP_INTERNAL_SERVER_ERROR)
              .getDescription());
    req.sendError(HTTPStatus::HTTP_INTERNAL_SERVER_ERROR);
  }

  handler.endRequest(req);
  return false;
}


void HTTP::expireCB() {
  double now = Timer::now();
  unsigned count;

  for (auto it = connections.begin(); it != connections.end();)
    if (maxConnectionTTL < now - (*it)->getStartTime()) {
      it = connections.erase(it);
      count++;

    } else it++;

  LOG_DEBUG(4, "Dropped " << count << " expired connections");
}


void HTTP::acceptCB() {
  IPAddress peer;
  auto newSocket = socket->accept(&peer);
  newSocket->setBlocking(false);

  LOG_DEBUG(4, "New connection from " << peer);

  // Create new Connection
  SmartPointer<Connection> con =
    new Connection(base, true, peer, newSocket->adopt(), sslCtx);

  con->setHTTP(this);
  con->setMaxHeaderSize(maxHeaderSize);
  con->setMaxBodySize(maxBodySize);
  if (0 <= priority) con->setPriority(priority);
  con->setReadTimeout(readTimeout);
  con->setWriteTimeout(writeTimeout);

  connections.push_back(con);

  // Send "service unavailable" at connection limit
  if (maxConnections && maxConnections < connections.size())
    con->sendServiceUnavailable();

  else con->acceptRequest();
}
