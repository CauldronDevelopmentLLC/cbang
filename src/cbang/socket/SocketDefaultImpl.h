/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include "SocketImpl.h"

#include <cbang/SmartPointer.h>

#include <iostream>

namespace cb {
  /// Create a TCP socket connection.
  class SocketDefaultImpl : public SocketImpl {
    socket_t socket;
    bool blocking;
    bool connected;

    SmartPointer<std::iostream> in;
    SmartPointer<std::iostream> out;

    // Don't allow copy constructor or assignment
    SocketDefaultImpl(const SocketDefaultImpl &o) : SocketImpl(0) {}
    SocketDefaultImpl &operator=(const SocketDefaultImpl &o) {return *this;}

  public:
    SocketDefaultImpl(Socket *parent);

    // From SocketImpl
    bool isOpen() const override;
    void setReuseAddr(bool reuse) override;
    void setBlocking(bool blocking) override;
    bool getBlocking() const override {return blocking;}
    void setKeepAlive(bool keepAlive) override;
    void setSendBuffer(int size) override;
    void setReceiveBuffer(int size) override;
    void setReceiveLowWater(int size) override;
    void setSendTimeout(double timeout) override;
    void setReceiveTimeout(double timeout) override;
    void open() override;
    void bind(const IPAddress &ip) override;
    void listen(int backlog) override;
    SmartPointer<Socket> accept(IPAddress *ip) override;
    void connect(const IPAddress &ip) override;
    std::streamsize write(
      const char *data, std::streamsize length, unsigned flags) override;
    std::streamsize read(
      char *data, std::streamsize length, unsigned flags) override;
    void close() override;
    socket_t get() const override {return socket;}
    void set(socket_t socket) override;
    socket_t adopt() override;

  protected:
    void capture(const IPAddress &addr, bool incoming);
  };
}
