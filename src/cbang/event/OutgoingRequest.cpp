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

#include <cbang/String.h>
#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/os/SysError.h>
#include <cbang/openssl/SSLContext.h>
#include <cbang/socket/Socket.h>
#include <cbang/time/Timer.h>

using namespace std;
using namespace cb;
using namespace cb::Event;


#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX << "OUT" << Connection::getID() << ':'


OutgoingRequest::OutgoingRequest(Client &client, const URI &uri,
                                 RequestMethod method, callback_t cb) :
  Connection(client.getBase(), false, uri.getIPAddress(), 0,
             uri.getScheme() == "https" ? client.getSSLContext() : 0),
  Request(method, uri), dns(client.getDNS()), cb(cb) {
  LOG_DEBUG(5, "Connecting to " << uri.getHost() << ':' << uri.getPort());
}


OutgoingRequest::~OutgoingRequest() {}


void OutgoingRequest::setProgressCallback(progress_cb_t cb, double delay) {
  progressCB = cb;
  progressDelay = delay;
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
  Connection::makeRequest(*this);
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
    LOG_ERROR("< " << error);

    string sslErrors = getSSLErrors();
    if (!sslErrors.empty()) sslErrors = " SSL:" + sslErrors;

    LOG_DEBUG(4, "SYS:" << SysError() << sslErrors);

  } else {
    LOG_INFO(1, "< " << getResponseLine());
    LOG_DEBUG(5, getInputHeaders() << '\n');
    LOG_DEBUG(6, getInputBuffer().hexdump() << '\n');
  }

  setConnectionError(error);
  if (cb) TRY_CATCH_ERROR(cb(*this));

  setConnection(0); // Release self reference
}
