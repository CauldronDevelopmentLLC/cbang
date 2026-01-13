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

#include "StreamEventBuffer.h"
#include "Event.h"

#include <cbang/net/Socket.h>

using namespace cb;
using namespace cb::Event;


StreamEventBuffer::StreamEventBuffer(
  Base &base, socket_t handle, unsigned flags) :
  StreamEventHandler(base, handle, flags), handle(handle) {}


bool StreamEventBuffer::isOpen() const {return handle != (socket_t)-1;}


void StreamEventBuffer::close() {
  if (isOpen()) Socket::close(handle);
  event->del();
  handle = (socket_t)-1;
}


void StreamEventBuffer::read() {
  if (isOpen()) read(event->getFD(), 1e6);
}


void StreamEventBuffer::write() {
  if (!isOpen()) return;
  write(event->getFD(), 1e6);
  if (!getLength()) close();
}


void StreamEventBuffer::onEvent(Event &event, int fd, unsigned flags) {
  if (flags & EVENT_READ)   read();
  if (flags & EVENT_WRITE) write();
}
