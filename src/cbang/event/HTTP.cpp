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

#include "HTTP.h"
#include "Base.h"
#include "Request.h"
#include "HTTPHandler.h"
#include "HTTPStatus.h"
#include "Headers.h"
#include "Buffer.h"
#include "BufferEvent.h"

#include <cbang/Exception.h>
#include <cbang/net/IPAddress.h>
#include <cbang/log/Logger.h>
#include <cbang/util/DefaultCatch.h>

#include <event2/http.h>
#include <event2/bufferevent.h>

#ifdef HAVE_OPENSSL
#include <cbang/openssl/SSLContext.h>

#include <event2/bufferevent_ssl.h>

#include <openssl/ssl.h>

#else
namespace cb {class SSLContext {};}
#endif

using namespace std;
using namespace cb::Event;


namespace {
  class RequestIntercept : public Base {
    HTTP &http;
    cb::SmartPointer<HTTPHandler> handler;

  public:
    RequestIntercept(HTTP &http, const cb::SmartPointer<HTTPHandler> &handler) :
      http(http), handler(handler) {}

    void request(evhttp_request *req) {http.request(*handler, req);}
  };


  bufferevent *bev_cb(event_base *base, void *arg) {
    try {return ((HTTP *)arg)->bevCB(base);} CATCH_ERROR;
    return 0;
  }


  void request_free_cb(evhttp_request *_req, void *arg) {
    try {delete (Request *)arg;} CATCH_ERROR;
  }


  void request_cb(evhttp_request *req, void *cb) {
    try {((RequestIntercept *)cb)->request(req);} CATCH_ERROR;
  }
}


HTTP::HTTP(const Base &base) : http(evhttp_new(base.getBase())), priority(-1) {
  if (!http) THROW("Failed to create event HTTP");

  evhttp_set_bevcb(http, bev_cb, this);
  evhttp_set_allowed_methods(http, ~0); // Allow all methods
}


HTTP::HTTP(const Base &base, const cb::SmartPointer<cb::SSLContext> &sslCtx) :
  http(evhttp_new(base.getBase())), sslCtx(sslCtx), priority(-1) {
  if (!http) THROW("Failed to create event HTTP");

  evhttp_set_bevcb(http, bev_cb, this);
  evhttp_set_allowed_methods(http, ~0); // Allow all methods

#ifndef HAVE_OPENSSL
  if (!sslCtx.isNull()) THROW("C! was not built with openssl support");
#endif
}


HTTP::~HTTP() {if (http) evhttp_free(http);}


void HTTP::setMaxBodySize(unsigned size) {evhttp_set_max_body_size(http, size);}


void HTTP::setMaxHeadersSize(unsigned size) {
  evhttp_set_max_headers_size(http, size);
}


void HTTP::setTimeout(int timeout) {evhttp_set_timeout(http, timeout);}
void HTTP::setMaxConnections(int x) {evhttp_set_max_connections(http, x);}
int HTTP::getConnectionCount() const {return evhttp_get_connection_count(http);}


void HTTP::setCallback(const string &path,
                       const cb::SmartPointer<HTTPHandler> &cb) {
  cb::SmartPointer<RequestIntercept> intercept =
    new RequestIntercept(*this, cb);

  int ret = evhttp_set_cb(http, path.c_str(), request_cb, intercept.get());
  if (ret)
    THROWS("Failed to set callback on path '" << path << '"'
           << (ret == -1 ? ", Already exists" : ""));
  requestCallbacks.push_back(intercept);
}


void HTTP::setGeneralCallback(const cb::SmartPointer<HTTPHandler> &cb) {
  cb::SmartPointer<RequestIntercept> intercept =
    new RequestIntercept(*this, cb);

  evhttp_set_gencb(http, request_cb, intercept.get());
  requestCallbacks.push_back(intercept);
}


int HTTP::bind(const cb::IPAddress &addr) {
  evhttp_bound_socket *handle =
    evhttp_bind_socket_with_handle(http, addr.getHost().c_str(),
                                   addr.getPort());

  if (!handle) THROWS("Unable to bind HTTP server to " << addr);

  return (int)evhttp_bound_socket_get_fd(handle);
}


bufferevent *HTTP::bevCB(event_base *base) {
  bufferevent *bev;

#ifdef HAVE_OPENSSL
  if (!sslCtx.isNull())
    bev = bufferevent_openssl_socket_new(base, -1, SSL_new(sslCtx->getCTX()),
                                         BUFFEREVENT_SSL_ACCEPTING,
                                         BEV_OPT_CLOSE_ON_FREE);
  else
#endif
    bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

  if (0 <= priority) BufferEvent(bev, false).setPriority(priority);

  return bev;
}


void HTTP::request(HTTPHandler &handler, evhttp_request *_req) {
  LOG_DEBUG(5, "New request, connection count = " << getConnectionCount());

  // NOTE, Request gets deallocated in request_free_cb() above
  Request *req = 0;

  try {
    // Allocate request
    req = handler.createRequest(_req);
    if (!req) THROW("Failed to create Event::Request");

    // Set deallocator
    evhttp_request_set_on_free_cb(_req, request_free_cb, req);

    // Set to incoming
    req->setIncoming(true);

    // Dispatch request
    try {
      if (!handler(*req)) req->sendError(HTTPStatus::HTTP_NOT_FOUND);

    } catch (cb::Exception &e) {
      req->sendError(e.getCode(), e.getMessage());
      if (!CBANG_LOG_DEBUG_ENABLED(3)) LOG_WARNING(e.getMessage());
      LOG_DEBUG(3, e);

    } catch (std::exception &e) {
      req->sendError(HTTPStatus::HTTP_INTERNAL_SERVER_ERROR, e.what());
      LOG_ERROR(e.what());

    } catch (...) {
      req->sendError(HTTPStatus::HTTP_INTERNAL_SERVER_ERROR);
      LOG_ERROR(HTTPStatus(HTTPStatus::HTTP_INTERNAL_SERVER_ERROR)
                .getDescription());
    }
  } CATCH_ERROR;

  if (req) handler.endRequestEvent(req);
}
