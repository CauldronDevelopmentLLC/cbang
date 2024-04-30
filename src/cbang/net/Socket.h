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

#pragma once

#include "SocketType.h"
#include "SockAddr.h"

#include <cbang/SmartPointer.h>

#include <ios>
#include <cstdint>


namespace cb {
  class SSL;
  class SSLContext;

  /// Create a TCP socket connection.
  class Socket {
    static bool initialized;

    socket_t socket = -1;
    bool blocking   = true;
    bool connected  = false;

    // Don't allow copy constructor or assignment
    Socket(const Socket &o) {}
    Socket &operator=(const Socket &o) {return *this;}

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
      NONBLOCKING = 1 << 0,
      PEEK        = 1 << 1,
    };


    Socket();
    virtual ~Socket();

    static void initialize();

    virtual SmartPointer<Socket> create() {return new Socket;}

    virtual SSL &getSSL() {THROW("Not a secure socket");}
    virtual SSLContext &getSSLContext() {THROW("Not a secure socket");}

    virtual bool isSecure() {return false;}
    virtual bool isOpen() const;
    virtual bool isConnected() const {return canWrite(0);}

    virtual bool canRead(double timeout = 0) const;
    virtual bool canWrite(double timeout = 0) const;

    virtual void setReuseAddr(bool reuse);
    virtual void setBlocking(bool blocking);
    virtual bool getBlocking() const {return blocking;}
    virtual void setCloseOnExec(bool closeOnExec);
    virtual void setKeepAlive(bool keepAlive);
    virtual void setSendBuffer(int size);
    virtual void setReceiveBuffer(int size);
    virtual void setReceiveLowWater(int size);
    virtual void setReceiveTimeout(double timeout);
    virtual void setSendTimeout(double timeout);
    virtual void setTimeout(double timeout);

    virtual void open(bool upd = false, bool ipv6 = false);
    virtual void bind(const SockAddr &addr);
    virtual void listen(int backlog = -1);
    virtual SmartPointer<Socket> accept(SockAddr &addr);

    /// Connect to the specified address and port
    virtual void connect(const SockAddr &addr,
                         const std::string &hostname = "");

    /// Write data to an open connection.
    std::streamsize write(
      const uint8_t *data, std::streamsize length, unsigned flags = 0,
      const SockAddr *addr = 0);

    /// Read data from an open connection.
    std::streamsize read(
      uint8_t *data, std::streamsize length, unsigned flags = 0,
      SockAddr *addr = 0);

    static void close(socket_t socket);
    virtual void close();

    virtual socket_t get() const {return socket;}
    virtual void set(socket_t socket);
    virtual socket_t adopt();

  protected:
    virtual std::streamsize writeImpl(
      const uint8_t *data, std::streamsize length, unsigned flags,
      const SockAddr *addr);

    /// Read data from an open connection.
    virtual std::streamsize readImpl(
      uint8_t *data, std::streamsize length, unsigned flags, SockAddr *addr);

    void assertOpen();
  };
}
