/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#include "OutgoingRequest.h"
#include "Client.h"
#include "Buffer.h"
#include "Headers.h"
#include "HTTPConnOut.h"

#include <cbang/String.h>
#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/os/SysError.h>
#include <cbang/socket/Socket.h>
#include <cbang/time/Timer.h>
#include <cbang/openssl/SSLContext.h>


using namespace std;
using namespace cb;
using namespace cb::Event;


#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX << "OUT" << getID() << ':'


OutgoingRequest::OutgoingRequest(Client &client, const URI &uri,
                                 RequestMethod method, callback_t cb) :
  Request(method, uri), client(client), cb(cb) {
  LOG_DEBUG(5, "Connecting to " << uri.getHost() << ':' << uri.getPort());

  // Create connection
  setConnection(new HTTPConnOut(client));
}


OutgoingRequest::~OutgoingRequest() {}


HTTPConnOut &OutgoingRequest::getConnection() {
  return *Request::getConnection().cast<HTTPConnOut>();
}


const HTTPConnOut &OutgoingRequest::getConnection() const {
  return *Request::getConnection().cast<HTTPConnOut>();
}


void OutgoingRequest::setProgressCallback(progress_cb_t cb, double delay) {
  progressCB = cb;
  progressDelay = delay;
}


void OutgoingRequest::connect(std::function<void (bool)> cb) {
  if (getConnection().isConnected()) {
    if (cb) cb(true);
    return;
  }

  getConnection().connect(client.getDNS(), getURI().getIPAddress(),
                          client.getBindAddress(), cb);
}


void OutgoingRequest::send() {
  // Set output headers
  if (!outHas("Host")) outSet("Host", getURI().getHost());
  if (!outHas("Connection")) outSet("Connection", "close");

  // Set Content-Length
  if (mayHaveBody() && !outHas("Content-Length"))
    outSet("Content-Length", String(getOutputBuffer().getLength()));

  LOG_INFO(1, "> " << getRequestLine());
  LOG_DEBUG(5, getOutputHeaders() << '\n');
  LOG_DEBUG(6, getOutputBuffer().hexdump() << '\n');

  // Do it
  SmartPointer<Request> req = this; // Keep the request alive
  connect([this, req] (bool success) {
            if (success) getConnection().makeRequest(this);
            else if (cb) cb(*this);
          });
}


void OutgoingRequest::onProgress(unsigned bytes, int total) {
  double now = Timer::now();

  if (progressCB && ((int)bytes == total || lastProgress + progressDelay < now))
    try {
      progressCB(bytes, total);
      lastProgress = now;
    } CATCH_ERROR;
}


void OutgoingRequest::onResponse(ConnectionError error) {
  if (error) {
    LOG_ERROR("< " << getConnection().getPeer() << ' ' << error);

    //string sslErrors = getSSLErrors(); TODO
    //if (!sslErrors.empty()) sslErrors = " SSL:" + sslErrors;

    //LOG_DEBUG(4, "SYS:" << SysError() << sslErrors);

  } else {
    LOG_INFO(1, "< " << getConnection().getPeer() << ' ' << getResponseLine());
    LOG_DEBUG(5, getInputHeaders() << '\n');
    LOG_DEBUG(6, getInputBuffer().hexdump() << '\n');
  }

  setConnectionError(error);
  if (cb) TRY_CATCH_ERROR(cb(*this));
}
