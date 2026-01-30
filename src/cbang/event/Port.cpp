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

#include "Port.h"
#include "Server.h"
#include "Event.h"

#include <cbang/net/Socket.h>
#include <cbang/log/Logger.h>
#include <cbang/os/SysError.h>

using namespace cb::Event;
using namespace cb;
using namespace std;


Port::Port(Server &server, const SockAddr addr,
           const SmartPointer<SSLContext> &sslCtx, int priority) :
  server(server), addr(addr), sslCtx(sslCtx), priority(priority) {}


Port::~Port() {}


void Port::setPriority(int priority) {
  this->priority = priority;
  if (event.isSet()) event->setPriority(priority);
}


void Port::open() {
  socket = new Socket;
  socket->open(Socket::NONBLOCKING | Socket::REUSEADDR, addr);
  socket->listen(server.getConnectionBacklog());
  addEvent();
}


void Port::close() {
  event->del();
  event.release();
  socket.release();
}


void Port::activate() {event->add();}


void Port::accept() {
  while (true) {
    if (server.getMaxConnections() <= server.getConnectionCount())
      return event->del();

    SockAddr peerAddr;
    SmartPointer<Socket> newSocket;
    try {
      newSocket = socket->accept(peerAddr, Socket::NONBLOCKING);
      if (newSocket.isNull()) return;
      backoff.reset();

    } catch (const Exception &e) {
      LOG_WARNING(e);

      // In case of accept() error, delay.  This ensures we don't get caught
      // in a tight loop with errors like EMFILE.
      return addEvent(backoff.next());
    }

    try {
      server.accept(peerAddr, newSocket, sslCtx);

    } catch (const SSLException &e) {
      LOG_DEBUG(4, e.getMessage());
    }
  }
}


void Port::addEvent(double delay) {
  if (event.isSet()) event->del();

  if (delay) event = server.getBase().newEvent([this] {addEvent();}, 0);
  else event = server.getBase().newEvent(
    socket->get(), [this] {accept();}, EVENT_READ | EVENT_PERSIST);

  if (0 <= priority) event->setPriority(priority);

  if (delay) event->add(delay);
  else event->add();
}
