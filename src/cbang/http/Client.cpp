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

#include "Client.h"
#include "ConnOut.h"
#include "ProxyRequest.h"

#include <cbang/openssl/SSLContext.h>
#include <cbang/os/SystemInfo.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


Client::Client(
  Event::Base &base, const SmartPointer<SSLContext> &sslCtx) :
  base(base), sslCtx(sslCtx) {}


Client::~Client() {}


void Client::send(const SmartPointer<Request> &req) const {
  auto &uri = req->getURI();

  if (!req->hasConnection()) req->setConnection(new ConnOut(base));

  // Configure connection
  auto &conn = req->getConnection();
  if (!conn->getStats().isSet()) conn->setStats(stats);
  conn->setReadTimeout(readTimeout);
  conn->setWriteTimeout(writeTimeout);

  // Check if already connected
  if (req->isConnected()) return conn->queueRequest(req);

  // SSL
  SmartPointer<SSLContext> sslCtx;
  if (uri.schemeRequiresSSL()) {
    sslCtx = getSSLContext();
    if (sslCtx.isNull()) THROW("Client lacks SSLContext");
  }

  // Proxy
  URI connectURI = uri;
  auto proxy = SystemInfo::instance().getProxy(uri);
  if (proxy.getScheme() == "http") {
    conn->queueRequest(new ProxyRequest(proxy, SmartPhony(req.get()), sslCtx));
    connectURI = proxy;

  } else {
    if (!proxy.getScheme().empty())
      THROW("Proxy scheme '" << proxy.getScheme() << "' not supported");

    if (sslCtx.isSet()) conn->openSSL(*sslCtx, uri.getHost());
  }

  // Queue request
  conn->queueRequest(SmartPhony(req.get())); // Don't create circular ref

  // Connect
  conn->connect(connectURI.getHost(), connectURI.getPort(), bindAddr);
}


Client::RequestPtr Client::call(
  const URI &uri, Method method, const char *data, unsigned length,
  callback_t cb) {
  auto con = SmartPtr(new ConnOut(base));
  auto req = SmartPtr(new OutgoingRequest(*this, con, uri, method, cb));

  if (data) req->getOutputBuffer().add(data, length);

  return req;
}


Client::RequestPtr Client::call(
  const URI &uri, Method method, const string &data, callback_t cb) {
  return call(uri, method, data.data(), data.length(), cb);
}


Client::RequestPtr Client::call(
  const URI &uri, Method method, callback_t cb) {
  return call(uri, method, 0, 0, cb);
}
