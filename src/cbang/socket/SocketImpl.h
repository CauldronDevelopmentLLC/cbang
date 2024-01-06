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

#include <cbang/SmartPointer.h>
#include <cbang/net/IPAddress.h>

#include <ios>
#include <climits>


namespace cb {
  class Socket;
  class SSL;
  class SSLContext;

  /// Socket implementation interface
  class SocketImpl {
  protected:
    Socket *parent;

  public:
    SocketImpl(Socket *parent) : parent(parent) {}
    virtual ~SocketImpl() {}

    Socket *getSocket() {return parent;}

    virtual SSL &getSSL() {THROW("Not a secure socket");}
    virtual SSLContext &getSSLContext() {THROW("Not a secure socket");}
    virtual bool isSecure() {return false;}
    virtual Socket *createSocket();
    virtual bool isOpen() const = 0;
    virtual void setReuseAddr(bool reuse) = 0;
    virtual void setBlocking(bool blocking) = 0;
    virtual bool getBlocking() const = 0;
    virtual void setKeepAlive(bool keepAlive) {}
    virtual void setSendBuffer(int size) {}
    virtual void setReceiveBuffer(int size) {}
    virtual void setReceiveLowWater(int size) {}
    virtual void setSendTimeout(double timeout) {}
    virtual void setReceiveTimeout(double timeout) {}
    virtual void open() = 0;
    virtual void bind(const IPAddress &ip) = 0;
    virtual void listen(int backlog) = 0;
    virtual SmartPointer<Socket> accept(IPAddress *ip) = 0;
    virtual void connect(const IPAddress &ip) = 0;
    virtual std::streamsize write(const char *data, std::streamsize length,
                                  unsigned flags) = 0;
    virtual std::streamsize read(char *data, std::streamsize length,
                                 unsigned flags) = 0;
    virtual void close() = 0;
    virtual socket_t get() const = 0;
    virtual void set(socket_t socket) = 0;
    virtual socket_t adopt() = 0;
  };
}
