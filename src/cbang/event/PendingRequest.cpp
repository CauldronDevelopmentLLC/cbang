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

#include "PendingRequest.h"
#include "Client.h"
#include "Buffer.h"
#include "Headers.h"

#include <cbang/String.h>
#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/debug/Debugger.h>
#include <cbang/os/SysError.h>

#include <event2/http.h>

using namespace std;
using namespace cb;
using namespace cb::Event;


namespace {
  void request_cb(struct evhttp_request *req, void *pr) {
    ((PendingRequest *)pr)->callback();
  }


  void error_cb(enum evhttp_request_error error, void *pr) {
    ((PendingRequest *)pr)->error(error);
  }
}


PendingRequest::PendingRequest(Client &client, const URI &uri, unsigned method,
                               callback_t cb) :
  Connection(client.getBase(), client.getDNS(), uri, client.getSSLContext()),
  Request(evhttp_request_new(request_cb, this), uri, true), client(client),
  method(method), cb(cb), err(0) {
  evhttp_request_set_error_cb(req.access(), error_cb);
}


PendingRequest::~PendingRequest() {
  LOG_DEBUG(6, __func__ << "() " << Debugger::getStackTrace());
  if (req.isSet()) evhttp_request_set_error_cb(req.access(), 0);
}


void PendingRequest::send() {
  // Set output headers
  if (!outHas("Host")) outSet("Host", getURI().getHost());
  if (!outHas("Connection")) outSet("Connection", "close");

  // Set Content-Length
  switch (method) {
  case HTTP_POST:
  case HTTP_PUT:
  case HTTP_PATCH:
    if (!outHas("Content-Length"))
      outSet("Content-Length", String(getOutputBuffer().getLength()));
    break;
  default: break;
  }

  LOG_INFO(1, "> " << getMethod() << " " << getURI());
  LOG_DEBUG(5, getOutputHeaders() << '\n');
  LOG_DEBUG(6, getOutputBuffer().hexdump() << '\n');

  // Do it
  err = 0;
  selfRef = this; // Keep alive during request
  Connection::makeRequest(req.access(), method, getURI());
}


void PendingRequest::callback() {
  try {
    if (!req) {
      if (cb) cb(0, err);
      selfRef.release();
      return;
    }

    LOG_DEBUG(5, getResponseLine() << '\n' << getInputHeaders() << '\n');
    LOG_DEBUG(6, getInputBuffer().hexdump() << '\n');

    if (cb) cb(this, err);
  } CATCH_ERROR;

  selfRef.release();
}


void PendingRequest::error(int code) {
  string sslErrors = getSSLErrors();

  LOG_ERROR("Request failed: " << getErrorStr(code)
            << ", System error: " << SysError()
            << (sslErrors.empty() ? string() :
                ", SSL errors: " + sslErrors));

  err = code;
}
