/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include "Socket.h"

#include "Winsock.h"
#include "SocketSet.h"

#include <cbang/config.h>

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/os/SysError.h>
#include <cbang/log/Logger.h>
#include <cbang/time/Timer.h>

#ifdef _WIN32
#include "Winsock.h"

typedef int socklen_t;  // Unix socket length
#define MSG_DONTWAIT 0
#define MSG_NOSIGNAL 0
#define SHUT_RDWR 2
#define SOCKET_INPROGRESS WSAEWOULDBLOCK

#else // _WIN32
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define INVALID_SOCKET -1        // WinSock invalid socket
#define SOCKET_ERROR   -1        // Basic WinSock error
#define SOCKET_INPROGRESS EINPROGRESS

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#endif

#include <cerrno>
#include <ctime>
#include <cstring>

using namespace std;
using namespace cb;


bool Socket::initialized = false;


Socket::Socket() {initialize();}
Socket::~Socket() {close();}


void Socket::initialize() {
  if (initialized) return;

#ifdef _WIN32
  WSADATA wsa;

  if (WSAStartup(MAKEWORD(2, 2), &wsa) != NO_ERROR)
    THROW("Failed to start winsock");

  if (LOBYTE(wsa.wVersion) != 2 || HIBYTE(wsa.wVersion) != 2)
    THROW("Error need winsock version 2.2");
#endif

  initialized = true;
}


bool Socket::isOpen() const {return socket != INVALID_SOCKET;}


bool Socket::canRead(double timeout) const {
  if (!isOpen()) return false;
  SocketSet set;
  set.add(*this, SocketSet::READ);
  return set.select(timeout);
}


bool Socket::canWrite(double timeout) const {
  if (!isOpen()) return false;
  SocketSet set;
  set.add(*this, SocketSet::WRITE);
  return set.select(timeout);
}


void Socket::setReuseAddr(bool reuse) {
  assertOpen();

#ifdef _WIN32
  BOOL opt = reuse;
#else
  int opt = reuse;
#endif

  SysError::clear();
  if (setsockopt((socket_t)socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                 sizeof(opt)))
    THROW("Failed to set reuse addr: " << SysError());
}


void Socket::setBlocking(bool blocking) {
  assertOpen();

#ifdef _WIN32
  u_long on = blocking ? 0 : 1;
  ioctlsocket((socket_t)socket, FIONBIO, &on);

#else
  int opts = fcntl(socket, F_GETFL);
  if (opts >= 0) {
    if (blocking) opts &= ~O_NONBLOCK;
    else opts |= O_NONBLOCK;

    fcntl(socket, F_SETFL, opts);
  }
#endif

  this->blocking = blocking;
}


void Socket::setCloseOnExec(bool closeOnExec) {
#ifndef _WIN32
  fcntl(socket, F_SETFD, closeOnExec ? FD_CLOEXEC : 0);
#endif
}


void Socket::setKeepAlive(bool keepAlive) {
  assertOpen();

#ifdef _WIN32
  BOOL opt = keepAlive;
#else
  int opt = keepAlive;
#endif

  SysError::clear();
  if (setsockopt((socket_t)socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt,
                 sizeof(opt)))
    THROW("Failed to set socket keep alive: " << SysError());
}


void Socket::setSendBuffer(int size) {
  assertOpen();

  if (setsockopt((socket_t)socket, SOL_SOCKET, SO_SNDBUF, (char *)&size,
                 sizeof(size)))
    THROW("Could not set send buffer to " << size << ": " << SysError());
}


void Socket::setReceiveBuffer(int size) {
  assertOpen();

  if (setsockopt((socket_t)socket, SOL_SOCKET, SO_RCVBUF, (char *)&size,
                 sizeof(size)))
    THROW("Could not set receive buffer to " << size << ": " << SysError());
}


void Socket::setReceiveLowWater(int size) {
  assertOpen();

  if (setsockopt((socket_t)socket, SOL_SOCKET, SO_RCVLOWAT, (char *)&size,
                 sizeof(size)))
    THROW("Could not set receive low water to " << size << ": " << SysError());
}


void Socket::setReceiveTimeout(double timeout) {
  assertOpen();

#ifdef _WIN32
  DWORD t = 1000 * timeout; // ms
#else
  struct timeval t = Timer::toTimeVal(timeout);
#endif

  if (setsockopt((socket_t)socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&t,
                 sizeof(t)))
    THROW("Could not set receive timeout to " << timeout << ": "
           << SysError());
}


void Socket::setSendTimeout(double timeout) {
  assertOpen();

#ifdef _WIN32
  DWORD t = 1000 * timeout; // ms
#else
  struct timeval t = Timer::toTimeVal(timeout);
#endif

  if (setsockopt(
        (socket_t)socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&t, sizeof(t)))
    THROW("Could not set send timeout to " << timeout << ": " << SysError());
}


void Socket::setTimeout(double timeout) {
  setReceiveTimeout(timeout);
  setSendTimeout(timeout);
}


void Socket::open(bool udp, bool ipv6) {
  if (isOpen()) THROW("Socket already open");

  int type = udp ? SOCK_DGRAM  : SOCK_STREAM;
  socket = ::socket(ipv6 ? AF_INET6 : AF_INET, type, 0);
  blocking = true;

  if (socket == INVALID_SOCKET)
    THROW("Failed to create socket: " << SysError());

  setCloseOnExec(true);
}


void Socket::bind(const SockAddr &addr) {
  assertOpen();
  addr.bind(socket);
}


void Socket::listen(int backlog) {
  assertOpen();

  SysError::clear();
  if (::listen(socket, backlog == -1 ? SOMAXCONN : backlog) == SOCKET_ERROR)
    THROW("listen failed");
}


SmartPointer<Socket> Socket::accept(SockAddr &addr) {
  assertOpen();

  socket_t s = addr.accept(socket);

  if (s != INVALID_SOCKET) {
    SmartPointer<Socket> aSock = create();
    aSock->socket = s;

    aSock->connected = true;
    aSock->setBlocking(blocking);

    LOG_DEBUG(5, "accept() new connection");

    return aSock;
  }

  return 0;
}


void Socket::connect(const SockAddr &addr, const string &hostname) {
  assertOpen();

  LOG_DEBUG(4, "Connecting to " << addr);

  try {
    addr.connect(socket);
    connected = true;

  } catch (const Exception &e) {
    close();

    throw;
  }
}


streamsize Socket::write(
  const uint8_t *data, streamsize length, unsigned flags,
  const SockAddr *addr) {
  assertOpen();
  bool blocking = !(flags & NONBLOCKING) && getBlocking();
  LOG_DEBUG(5, "Socket start " << (blocking ? "" : "non-")
            << "blocking write " << length);

  streamsize bytes = writeImpl(data, length, flags, addr);

  LOG_DEBUG(5, "Socket write " << bytes);
  if (bytes) LOG_DEBUG(6, String::hexdump(data, bytes));

  return bytes;
}


streamsize Socket::read(
  uint8_t *data, streamsize length, unsigned flags, SockAddr *addr) {
  assertOpen();
  bool blocking = !(flags & NONBLOCKING) && getBlocking();
  LOG_DEBUG(5, "Socket start " << (blocking ? "" : "non-")
            << "blocking read " << length);

  streamsize bytes = readImpl(data, length, flags, addr);

  LOG_DEBUG(5, "Socket read " << bytes);

  if (bytes == -1) throw EndOfStream();
  if (bytes) LOG_DEBUG(6, String::hexdump(data, bytes));

  return bytes;
}


void Socket::close(socket_t socket) {
  LOG_DEBUG(4, "Socket::close(" << socket << ")");

#ifdef _WIN32
  if (closesocket((SOCKET)socket))
#else
  if (::close(socket))
#endif
    LOG_ERROR("Closing socket " << socket << ": " << SysError());
}


void Socket::close() {
  if (!isOpen()) return;

  // If socket was connected call shutdown()
  if (connected) {
    shutdown(socket, SHUT_RDWR);
    connected = false;
  }

  close(socket);

  socket = INVALID_SOCKET;
}


void Socket::set(socket_t socket) {
  close();
  this->socket = socket;
}


socket_t Socket::adopt() {
  socket_t s = socket;
  socket = INVALID_SOCKET;
  connected = false;
  return s;
}


streamsize Socket::writeImpl(
  const uint8_t *data, streamsize length, unsigned flags,
  const SockAddr *addr) {
  assertOpen();
  if (!length) return 0;

  int f = MSG_NOSIGNAL;
  if (flags & Socket::NONBLOCKING) f |= MSG_DONTWAIT;

  SysError::clear();
  streamsize ret = sendto((socket_t)socket, (char *)data, length, f,
                          addr ? addr->get() : 0, addr ? addr->getLength() : 0);
  int err = SysError::get();

  LOG_DEBUG(5, "sendto() = " << ret << " of " << length);

  if (ret < 0) {
#ifdef _WIN32
    // NOTE: sendto() can return -1 even when there is no error
    if (!err || err == WSAEWOULDBLOCK || err == WSAENOBUFS) return 0;
#else
    if (err == EAGAIN) return 0;
#endif

    THROW("Send error: " << err << ": " << SysError(err));
  }

  return ret;
}


streamsize Socket::readImpl(
  uint8_t *data, streamsize length, unsigned flags, SockAddr *addr) {
  assertOpen();
  if (!length) return 0;

  int f = MSG_NOSIGNAL;
  if (flags & Socket::NONBLOCKING) f |= MSG_DONTWAIT;
  if (flags & Socket::PEEK) f |= MSG_PEEK;

  SysError::clear();
  socklen_t alen = addr ? addr->getCapacity() : 0;
  streamsize ret = recvfrom((socket_t)socket, (char *)data, length, f,
                            addr ? addr->get() : 0, addr ? &alen : 0);
  int err = SysError::get();

  LOG_DEBUG(5, "recvfrom() = " << ret << " of " << length);
  if (addr) LOG_DEBUG(5, "recvfrom() address " << *addr);

  if (!ret) return -1; // Orderly shutdown
  if (ret < 0) {
#ifdef _WIN32
    // NOTE: Windows can return -1 even when there is no error
    if (!err || err == WSAEWOULDBLOCK || err == WSAETIMEDOUT) return 0;
#else
    if (err == ECONNRESET) return -1;
    if (err == EAGAIN || err == EWOULDBLOCK) return 0;
#endif

    THROW("Receive error: " << err << ": " << SysError(err));
  }

  return ret;
}


void Socket::assertOpen() {if (!isOpen()) THROW("Socket not open");}
