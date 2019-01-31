/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2017, Cauldron Development LLC
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
    ((PendingRequest *)pr)->callback(req);
  }


  void error_cb(enum evhttp_request_error error, void *pr) {
    ((PendingRequest *)pr)->error(error);
  }


  void free_cb(struct evhttp_request *req, void *pr) {
    ((PendingRequest *)pr)->freed();
  }
}


PendingRequest::PendingRequest(Client &client, const URI &uri, unsigned method,
                               const SmartPointer<HTTPResponseHandler> &cb) :
  Connection(client.getBase(), client.getDNS(), uri, client.getSSLContext()),
  Request(evhttp_request_new(request_cb, this), uri, true), client(client),
  method(method), cb(cb), err(0) {
  evhttp_request_set_error_cb(req, error_cb);
  evhttp_request_set_on_free_cb(req, free_cb, this);
}


PendingRequest::~PendingRequest() {
  LOG_DEBUG(6, __func__ << "() " << Debugger::getStackTrace());

  if (req) {
    evhttp_request_set_error_cb(req, 0);
    evhttp_request_set_on_free_cb(req, 0, 0);
  }
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
  Connection::makeRequest(*this, method, getURI());

  if (err || !getRequest()) THROWS("Failed to send to " << getURI());

  SmartPointer<PendingRequest>::SelfRef::selfRef();
}


void PendingRequest::callback(evhttp_request *_req) {
  SmartPointer<PendingRequest> _ = this; // Don't deallocate while in callback

  try {
    if (!_req) {
      (*cb)(0, err);
      SmartPointer<PendingRequest>::SelfRef::selfDeref();
      return;
    }

    Request req(_req);

    LOG_DEBUG(5, req.getResponseLine() << '\n' << req.getInputHeaders()
              << '\n');
    LOG_DEBUG(6, req.getInputBuffer().hexdump() << '\n');

    (*cb)(&req, err);
  } CATCH_ERROR;

  SmartPointer<PendingRequest>::SelfRef::selfDeref();
}


void PendingRequest::error(int code) {
  string sslErrors = getSSLErrors();

  LOG_ERROR("Request failed: " << getErrorStr(code)
            << ", System error: " << SysError()
            << (sslErrors.empty() ? string() :
                ", SSL errors: " + sslErrors));

  err = code;
}


void PendingRequest::freed() {
  LOG_DEBUG(1, __func__ << "() " << Debugger::getStackTrace());

  unsigned refs = SmartPointer<PendingRequest>::SelfRef::getCount();
  bool engaged = SmartPointer<PendingRequest>::SelfRef::isSelfRefEngaged();

  if ((engaged && refs != 1) || (!engaged && refs != 0))
    LOG_ERROR("PendingRequest freed in libevent but ref count " << refs
              << " and self ref " << (engaged ? "" : "not") << " engaged");

  req = 0;
}
