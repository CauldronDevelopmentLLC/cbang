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

#pragma once

#include "SocketType.h"
#include "SockAddr.h"

#include <cbang/SmartPointer.h>
#include <cbang/util/NonCopyable.h>

#include <ios>
#include <cstdint>


namespace cb {
  class SSL;
  class SSLContext;

  /// Create a TCP socket connection.
  class Socket : public NonCopyable {
    static bool initialized;

    socket_t socket = -1;
    bool autoClose  = true;
    bool blocking   = true;
    bool connected  = false;

  public:
    // NOTE Inheriting from std::exception instead of cb::Exception due to
    // problems with matching the correct catch in SocketDevice::read() and
    // perhaps elsewhere.  Presumably this is caused by a compiler bug.
    class EndOfStream : public std::exception {
    public:
      EndOfStream() {}

      // From std::exception
      virtual const char *what() const throw() override
      {return "End of stream";}
    };


    enum {
      NONBLOCKING   = 1 << 0,
      PEEK          = 1 << 1,
      NOAUTOCLOSE   = 1 << 2,
      IPV6          = 1 << 3,
      UDP           = 1 << 4,
      NOCLOSEONEXEC = 1 << 5,
    };


    Socket();
    ~Socket();

    static void initialize();

    void setAutoClose(bool autoClose) {this->autoClose = autoClose;}

    bool isOpen() const;
    bool isConnected() const {return canWrite(0);}

    bool canRead(double timeout = 0) const;
    bool canWrite(double timeout = 0) const;

    void setReuseAddr(bool reuse);
    void setBlocking(bool blocking);
    bool getBlocking() const {return blocking;}
    void setCloseOnExec(bool closeOnExec);
    void setKeepAlive(bool keepAlive);
    void setSendBuffer(int size);
    void setReceiveBuffer(int size);
    void setReceiveLowWater(int size);
    void setReceiveTimeout(double timeout);
    void setSendTimeout(double timeout);
    void setTimeout(double timeout);

    void open(unsigned flags = 0);
    void bind(const SockAddr &addr);
    void listen(int backlog = -1);
    SmartPointer<Socket> accept(SockAddr &addr);

    /// Connect to the specified address and port
    void connect(const SockAddr &addr, const std::string &hostname = "");

    /// Write data to an open connection.
    std::streamsize write(
      const uint8_t *data, std::streamsize length, unsigned flags = 0,
      const SockAddr *addr = 0);

    /// Read data from an open connection.
    std::streamsize read(
      uint8_t *data, std::streamsize length, unsigned flags = 0,
      SockAddr *addr = 0);

    static void close(socket_t socket);
    void close();

    socket_t get() const {return socket;}

    void assertOpen();
  };
}
