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

#include "HTTP.h"
#include "Base.h"
#include "Request.h"
#include "HTTPStatus.h"
#include "Headers.h"
#include "Buffer.h"
#include "BufferEvent.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>
#include <cbang/Catch.h>

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
  bufferevent *bev_cb(event_base *base, void *arg) {
    TRY_CATCH_ERROR(return ((HTTP *)arg)->bevCB(base))
    return 0;
  }


  void request_cb(evhttp_request *req, void *arg) {
    TRY_CATCH_ERROR(((HTTP *)arg)->requestCB(req));
  }
}


HTTP::HTTP(const Base &base, const SmartPointer<HTTPHandler> &handler,
           const cb::SmartPointer<cb::SSLContext> &sslCtx) :
  http(evhttp_new(base.getBase())), handler(handler), sslCtx(sslCtx),
  priority(-1) {
  if (!http) THROW("Failed to create event HTTP");

  evhttp_set_bevcb(http, bev_cb, this);
  evhttp_set_allowed_methods(http, ~0); // Allow all methods
  evhttp_set_gencb(http, request_cb, this);

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
void HTTP::setMaxConnections(unsigned x) {evhttp_set_max_connections(http, x);}
int HTTP::getConnectionCount() const {return evhttp_get_connection_count(http);}


void HTTP::setMaxConnectionTTL(unsigned x) {
  evhttp_set_max_connection_ttl(http, x);
}


int HTTP::bind(const cb::IPAddress &addr) {
  evhttp_bound_socket *handle =
    evhttp_bind_socket_with_handle(http, addr.getHost().c_str(),
                                   addr.getPort());

  if (!handle) THROWS("Unable to bind HTTP server to " << addr);
  boundAddr = addr;

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


void HTTP::requestCB(evhttp_request *_req) {
  LOG_DEBUG(5, "New request on " << boundAddr << ", connection count = "
            << getConnectionCount());

  // NOTE, Request gets deallocated via SmartPointer<Request>::SelfRef
  Request *req = 0;

  try {
    // Allocate request
    req = handler->createRequest(_req);
    if (!req) THROW("Failed to create Event::Request");

    // Set to incoming
    req->setIncoming(true);

    // Dispatch request
    dispatch(*handler, *req);
  } CATCH_ERROR;

  if (req) handler->endRequest(req);
}


bool HTTP::dispatch(HTTPHandler &handler, Request &req) {
  try {
    if (handler.handleRequest(req)) return true;
    else req.sendError(HTTPStatus::HTTP_NOT_FOUND);

  } catch (cb::Exception &e) {
    if (!CBANG_LOG_DEBUG_ENABLED(3)) LOG_WARNING(e.getMessage());
    LOG_DEBUG(3, e);
    req.sendError(e.getCode(), e.getMessage());

  } catch (std::exception &e) {
    LOG_ERROR(e.what());
    req.sendError(HTTPStatus::HTTP_INTERNAL_SERVER_ERROR, e.what());

  } catch (...) {
    LOG_ERROR(HTTPStatus(HTTPStatus::HTTP_INTERNAL_SERVER_ERROR)
              .getDescription());
    req.sendError(HTTPStatus::HTTP_INTERNAL_SERVER_ERROR);
  }

  return false;
}
