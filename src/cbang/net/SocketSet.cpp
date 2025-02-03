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

#include "SocketSet.h"

#include "Winsock.h"
#include "Socket.h"
#include "SocketType.h"

#include <cbang/Exception.h>
#include <cbang/SmartPointer.h>
#include <cbang/time/Timer.h>
#include <cbang/os/SysError.h>

#ifndef _WIN32
#include <csignal>
#include <poll.h>
#endif


using namespace cb;


SocketSet::SocketSet() {}


void SocketSet::add(const Socket &socket, int type) {
  if (!socket.isOpen()) THROW("Socket not open");
  socket_t s = (socket_t)socket.get();

  auto ret = sockets.insert(sockets_t::value_type(s, type));
  if (!ret.second) ret.first->second |= type;
}


void SocketSet::remove(const Socket &socket, int type) {
  if (!socket.isOpen()) THROW("Socket not open");
  socket_t s = (socket_t)socket.get();

  auto ret = sockets.insert(sockets_t::value_type(s, 0));
  if (!ret.second) ret.first->second &= type;
}


bool SocketSet::isSet(const Socket &socket, int type) const {
  if (!socket.isOpen()) THROW("Socket not open");
  socket_t s = (socket_t)socket.get();

  auto it = sockets.find(s);
  return it != sockets.end() && it->second & type;
}


bool SocketSet::select(double timeout) {
#ifdef _WIN32
  int maxFD = -1;
  fd_set read;
  fd_set write;
  fd_set except;

  FD_ZERO(&read);
  FD_ZERO(&write);
  FD_ZERO(&except);

  for (auto &p: sockets) {
    if (maxFD < p.first)   maxFD = p.first;
    if (p.second & READ)   FD_SET(p.first, &read);
    if (p.second & WRITE)  FD_SET(p.first, &write);
    if (p.second & EXCEPT) FD_SET(p.first, &except);
  }

  struct timeval t;
  if (0 <= timeout) t = Timer::toTimeVal(timeout);

  SysError::clear();
  int ret =
    ::select(maxFD + 1, &read, &write, &except, 0 <= timeout ? &t : 0);

  if (ret < 0) THROW("select() " << SysError());

  for (auto &p: sockets) {
    p.second = 0;
    if (FD_ISSET(p.first, &read))   p.second |= READ;
    if (FD_ISSET(p.first, &write))  p.second |= WRITE;
    if (FD_ISSET(p.first, &except)) p.second |= EXCEPT;
  }

  return ret;

#else  // !_WIN32
  // Emulate select() with poll() on Linux because select() is unable to handle
  // file descriptors >= 1024.
  struct pollfd fds[sockets.size()];

  int i = 0;
  for (auto &p: sockets) {
    fds[i].fd = p.first;
    fds[i].events =
      ((p.second & READ)  ? POLLIN  : 0) |
      ((p.second & WRITE) ? POLLOUT : 0);
    fds[i].revents = 0;
    i++;
  }

  int ret = poll(fds, sockets.size(), timeout * 1000);

  if (ret < 0) THROW("poll() " << SysError());

  bool found = false;
  i = 0;
  for (auto &p: sockets) {
    int mask = p.second;
    p.second =
      ((fds[i].revents & POLLIN)               ? READ   : 0) |
      ((fds[i].revents & POLLOUT)              ? WRITE  : 0) |
      ((fds[i].revents & (POLLERR | POLLNVAL)) ? EXCEPT : 0);
    p.second &= mask;
    if (p.second) found = true;
    i++;
  }

  return found;

#endif // !_WIN32
}
