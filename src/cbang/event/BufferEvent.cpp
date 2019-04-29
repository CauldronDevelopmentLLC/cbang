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

#include "BufferEvent.h"
#include "Base.h"
#include "DNSBase.h"

#include <cbang/Exception.h>
#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/os/SysError.h>

#include <event2/bufferevent.h>
#include <event2/event.h>

#ifdef HAVE_OPENSSL
#include <cbang/openssl/SSL.h>
#include <cbang/openssl/SSLContext.h>

#include <event2/bufferevent_ssl.h>

#include <openssl/ssl.h>
#endif // HAVE_OPENSSL


extern "C"
int bufferevent_disable_hard_(struct bufferevent *bufev, short event);


using namespace std;
using namespace cb;
using namespace cb::Event;


namespace {
  void read_cb(struct bufferevent *bev, void *arg) {
    TRY_CATCH_ERROR(((BufferEvent *)arg)->readCB());
  }


  void write_cb(struct bufferevent *bev, void *arg) {
    TRY_CATCH_ERROR(((BufferEvent *)arg)->writeCB());
  }


  void event_cb(struct bufferevent *bev, short what, void *arg) {
    TRY_CATCH_ERROR(((BufferEvent *)arg)->eventCB(what));
  }
}


BufferEvent::BufferEvent(cb::Event::Base &base, bool incoming, socket_t fd,
                         const SmartPointer<SSLContext> &sslCtx) :
  bev(0), inputBuffer((evbuffer *)0), outputBuffer((evbuffer *)0) {

  if (sslCtx.isNull())
    bev = bufferevent_socket_new
      (base.getBase(), fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);

#ifdef HAVE_OPENSSL
  else
    bev = bufferevent_openssl_socket_new
      (base.getBase(), fd, SSL_new(sslCtx->getCTX()),
       incoming ? BUFFEREVENT_SSL_ACCEPTING : BUFFEREVENT_SSL_CONNECTING,
       BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);

#else // HAVE_OPENSSL
  else THROW("C! was not built with OpenSSL support");

#endif // HAVE_OPENSSL

  if (!bev) THROW("Failed to create buffer event");

  bufferevent_setcb(bev, read_cb, write_cb, event_cb, this);

#ifdef HAVE_OPENSSL
  if (sslCtx.isSet()) bufferevent_openssl_set_allow_dirty_shutdown(bev, 1);
#endif

  inputBuffer = bufferevent_get_input(bev);
  outputBuffer = bufferevent_get_output(bev);
}


BufferEvent::~BufferEvent() {
  if (!bev) return;

  bufferevent_disable(bev, EV_READ | EV_WRITE);
  bufferevent_setcb(bev, 0, 0, 0, 0);
  bufferevent_free(bev);
}


void BufferEvent::setFD(socket_t fd) {bufferevent_setfd(bev, fd);}
socket_t BufferEvent::getFD() const {return bufferevent_getfd(bev);}


void BufferEvent::setPriority(int priority) {
  bufferevent *bev = bufferevent_get_underlying(this->bev);
  if (!bev) bev = this->bev;

  if (bufferevent_priority_set(bev, priority))
    THROW("Unable to set buffer event priority to " << priority);
}


int BufferEvent::getPriority() const {return bufferevent_get_priority(bev);}


void BufferEvent::setTimeouts(unsigned read, unsigned write) {
  const struct timeval read_tv = {(time_t)read, 0};
  const struct timeval write_tv = {(time_t)write, 0};
  bufferevent_set_timeouts(bev, &read_tv, &write_tv);
}


bool BufferEvent::hasSSL() const {
#ifdef HAVE_OPENSSL
  return bufferevent_openssl_get_ssl(bev);
#else
  return false;
#endif
}


#ifdef HAVE_OPENSSL
cb::SSL BufferEvent::getSSL() const {
  struct ssl_st *ssl = bufferevent_openssl_get_ssl(bev);
  if (!ssl) THROW("BufferEvent does not have SSL");
  return cb::SSL(ssl);
}
#endif


void BufferEvent::logSSLErrors() {
#ifdef HAVE_OPENSSL
  string errors = getSSLErrors();
  if (!errors.empty()) LOG_ERROR("SSL errors: " << errors);
#endif
}


string BufferEvent::getSSLErrors() {
  string errors;

#ifdef HAVE_OPENSSL
  unsigned error;

  while ((error = bufferevent_get_openssl_error(bev))) {
    if (!errors.empty()) errors += ", ";
    errors += SSL::getErrorStr(error);
  }
#endif

  return errors;
}


bool BufferEvent::isWrapper() const {return bufferevent_get_underlying(bev);}


void BufferEvent::setRead(bool enabled, bool hard) {
  if (!enabled && hard) bufferevent_disable_hard_(bev, EV_READ);
  else (enabled ? bufferevent_enable : bufferevent_disable)(bev, EV_READ);
}


void BufferEvent::setWrite(bool enabled, bool hard) {
  if (!enabled && hard) bufferevent_disable_hard_(bev, EV_WRITE);
  else (enabled ? bufferevent_enable : bufferevent_disable)(bev, EV_WRITE);
}


void BufferEvent::setWatermark(bool read, size_t low, size_t high) {
  bufferevent_setwatermark(bev, read ? EV_READ : EV_WRITE, low, high);
}


void BufferEvent::connect(DNSBase &dns, const IPAddress &peer) {
  const char *hostname = peer.getHost().c_str();
  int port = peer.getPort();

  // TODO for some reason async DNS lookups fail with SSL connections

  if (bufferevent_socket_connect_hostname
      (bev, hasSSL() ? 0 : dns.getDNSBase(), AF_UNSPEC, hostname, port) < 0)
    THROW("Failed to connect to " << peer << ", System error: " << SysError());
}


string BufferEvent::getEventsString(short events) {
  vector<string> parts;

  if (events & BEV_EVENT_READING)   parts.push_back("READING");
  if (events & BEV_EVENT_WRITING)   parts.push_back("WRITING");
  if (events & BEV_EVENT_EOF)       parts.push_back("EOF");
  if (events & BEV_EVENT_ERROR)     parts.push_back("ERROR");
  if (events & BEV_EVENT_TIMEOUT)   parts.push_back("TIMEOUT");
  if (events & BEV_EVENT_CONNECTED) parts.push_back("CONNECTED");

  return String::join(parts, "|");
}
