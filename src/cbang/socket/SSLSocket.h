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

#include "Socket.h"
#include "SocketBIO.h"


namespace cb {
  class SSLSocket : public Socket {
    SocketBIO bio;
    SmartPointer<SSL> ssl;
    SmartPointer<SSLContext> sslCtx;

    bool inSSL = false;

    // Don't allow copy constructor or assignment
    SSLSocket(const SSLSocket &o) : bio(*this) {}
    SSLSocket &operator=(const SSLSocket &o) {return *this;}

  public:
    SSLSocket(const SmartPointer<SSLContext> &sslCtx);
    ~SSLSocket();

    // From Socket
    SmartPointer<Socket> create() override {return new SSLSocket(sslCtx);}
    SSL &getSSL() override {return *ssl;}
    SSLContext &getSSLContext() override {return *sslCtx;}
    bool isSecure() override {return true;}
    SmartPointer<Socket> accept(SockAddr &addr) override;
    void connect(const SockAddr &addr, const std::string &hostname) override;
    std::streamsize writeImpl(
      const uint8_t *data, std::streamsize length, unsigned flags,
      const SockAddr *addr) override;
    std::streamsize readImpl(
      uint8_t *data, std::streamsize length, unsigned flags,
      SockAddr *addr) override;
    void close() override;
  };
}
