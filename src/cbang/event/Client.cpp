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

#include "Client.h"
#include "HTTPConnOut.h"

#include <cbang/openssl/SSLContext.h>

using namespace std;
using namespace cb;
using namespace cb::Event;


Client::Client(cb::Event::Base &base, DNSBase &dns,
               const SmartPointer<SSLContext> &sslCtx) :
  base(base), dns(dns), sslCtx(sslCtx) {}


Client::~Client() {}


void Client::send(const SmartPointer<Request> &req) const {
  auto &uri = req->getURI();

  if (!req->hasConnection()) req->setConnection(new HTTPConnOut(base));
  auto &conn = req->getConnection();

  // Configure connection
  if (!conn->getStats().isSet()) conn->setStats(stats);
  conn->setReadTimeout(readTimeout);
  conn->setWriteTimeout(writeTimeout);

  // Check if already connected
  if (req->isConnected()) return conn->makeRequest(req);

  SmartPointer<SSLContext> sslCtx;
  if (uri.schemeRequiresSSL()) {
    sslCtx = getSSLContext();
    if (sslCtx.isNull()) THROW("Client lacks SSLContext");
  }

  // Connect
  conn->connect(
    dns, uri.getIPAddress(), bindAddr, sslCtx,
    [req] (bool success) {
      if (success) req->getConnection()->makeRequest(req);
      else req->onResponse(ConnectionError::CONN_ERR_CONNECT);
    });
}


Client::RequestPtr Client::call(
  const URI &uri, RequestMethod method, const char *data, unsigned length,
  callback_t cb) {
  auto req = SmartPtr(new OutgoingRequest(*this, uri, method, cb));

  req->setConnection(new HTTPConnOut(base));

  if (data) req->getOutputBuffer().add(data, length);

  return req;
}


Client::RequestPtr Client::call
(const URI &uri, RequestMethod method, const string &data, callback_t cb) {
  return call(uri, method, CPP_TO_C_STR(data), data.length(), cb);
}


Client::RequestPtr Client::call(
  const URI &uri, RequestMethod method, callback_t cb) {
  return call(uri, method, 0, 0, cb);
}
