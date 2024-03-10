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

#include <cbang/config.h>

#ifdef HAVE_OPENSSL

#include "SSLSocket.h"

#include <cbang/Exception.h>

#include <cbang/openssl/SSL.h>
#include <cbang/openssl/SSLContext.h>

#include <cbang/util/SmartToggle.h>

using namespace cb;
using namespace std;


SSLSocket::SSLSocket(const SmartPointer<SSLContext> &sslCtx) :
  bio(*this), ssl(sslCtx->createSSL(bio.getBIO())), sslCtx(sslCtx) {}


SSLSocket::~SSLSocket() {}


SmartPointer<Socket> SSLSocket::accept(SockAddr &addr) {
  SmartPointer<Socket> socket = Socket::accept(addr);
  if (socket.isNull()) return 0;

  auto impl = socket.cast<SSLSocket>();

  SmartToggle toggle(impl->inSSL);
  impl->ssl->accept();

  return socket;
}


void SSLSocket::connect(const SockAddr &addr, const string &hostname) {
  Socket::connect(addr);
  SmartToggle toggle(inSSL);
  if (!hostname.empty()) ssl->setTLSExtHostname(hostname);
  ssl->connect();
}


streamsize SSLSocket::writeImpl(
  const uint8_t *data, streamsize length, unsigned flags,
  const SockAddr *addr) {
  if (!length) return 0;

  if (inSSL) return Socket::writeImpl(data, length, flags, addr);
  else {
    if (addr) THROW("Address argument not supported in SSLSocket::write()");

    SmartToggle toggle(inSSL);
    streamsize ret = ssl->write((char *)data, length);

    if (SSL::peekError()) THROW("SSL read error " << SSL::getErrorStr());

    if (!bio.getException().isNull()) throw *bio.getException();
    return ret;
  }
}


streamsize SSLSocket::readImpl(
  uint8_t *data, streamsize length, unsigned flags, SockAddr *addr) {
  if (!length) return 0;

  if (inSSL) return Socket::readImpl(data, length, flags, addr);
  else {
    if (addr) THROW("Address argument not supported in SSLSocket::read()");

    SmartToggle toggle(inSSL);
    streamsize ret = ssl->read((char *)data, length);

    if (SSL::peekError()) THROW("SSL read error " << SSL::getErrorStr());

    if (!bio.getException().isNull()) throw *bio.getException();
    return ret;
  }
}


void SSLSocket::close() {
  if (isOpen()) {
    SmartToggle toggle(inSSL);
    ssl->shutdown();
  }

  Socket::close();
}

#endif // HAVE_OPENSSL
