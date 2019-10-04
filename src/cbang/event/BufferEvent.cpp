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
#include "Event.h"
#include "DNSBase.h"

#include <cbang/Exception.h>
#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/os/SysError.h>
#include <cbang/socket/Socket.h>

#include <event2/util.h>
#include <event2/event.h>
#include <event2/buffer.h>

#ifdef HAVE_OPENSSL
#include <cbang/openssl/SSL.h>
#include <cbang/openssl/SSLContext.h>

#include <openssl/ssl.h>
#endif // HAVE_OPENSSL

#ifdef HAVE_VALGRIND
#include <valgrind/memcheck.h>
#endif

#include <string.h> // For memset()

#define EVBUFFER_CB_NODEFER 2

#ifdef _WIN32
#define ERR_RW_RETRIABLE(e) ((e) == WSAEWOULDBLOCK || (e) == WSAEINTR)
#define ERR_CONNECT_RETRIABLE(e)                                        \
  (ERR_RW_RETRIABLE(e) || (e) == WSAEINPROGRESS || (e) == WSAEINVAL)

#else // _WIN32
#define ERR_CONNECT_RETRIABLE(e) ((e) == EINTR || (e) == EINPROGRESS)
#define ERR_RW_RETRIABLE(e)                             \
  ((e) == EINTR || (e) == EAGAIN || (e) == EWOULDBLOCK)
#endif // _WIN32

using namespace std;
using namespace cb;
using namespace cb::Event;


#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX << "BEV" << getID() << ':'


uint64_t BufferEvent::nextID = 0;


BufferEvent::BufferEvent(cb::Event::Base &base, bool incoming,
                         const SmartPointer<Socket> &socket,
                         const SmartPointer<SSLContext> &sslCtx) :
  base(base) {
  LOG_DEBUG(4, __func__ << "()");

  connectCBEvent = newEvent(&BufferEvent::connectCB);
  writeCBEvent   = newEvent(&BufferEvent::writeCB);
  readCBEvent    = newEvent(&BufferEvent::readCB);
  errorCBEvent   = newEvent(&BufferEvent::errorCB);

  if (sslCtx.isNull()) {
    outputBuffer.setFlags(EVBUFFER_FLAG_DRAINS_TO_FD);
    state = incoming ? STATE_SOCK_READY : STATE_IDLE;

  } else { // SSL
#ifdef HAVE_OPENSSL
    ssl = SSL_new(sslCtx->getCTX());

    // Configure SSL
    SSL_set_mode(ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

    if (incoming) SSL_set_accept_state(ssl);
    else SSL_set_connect_state(ssl);

    state = incoming ? STATE_SSL_HANDSHAKE : STATE_IDLE;

#else // HAVE_OPENSSL

    THROW("C! was not built with OpenSSL support");
#endif
  }

  setSocket(socket);
  outputBuffer.setCallback([this] (int added, int deleted, int orig) {
                             outputBufferCB(added, deleted, orig);
                           });
  updateEvents();
}


BufferEvent::~BufferEvent() {
  LOG_DEBUG(4, __func__ << "()");
  close();
  if (ssl) SSL_free(ssl);
}


void BufferEvent::setSocket(const SmartPointer<Socket> &socket) {
  setFD(socket.isNull() ? -1 : socket->get());
  this->socket = socket; // Note, setFD() releases socket
  if (socket.isSet()) socket->setBlocking(false);
}


const SmartPointer<Socket> &BufferEvent::getSocket() const {return socket;}



int BufferEvent::getPriority() const {return readCBEvent->getPriority();}


void BufferEvent::setPriority(int priority) {
  if (readEvent.isSet()) readEvent->setPriority(priority);
  if (writeEvent.isSet()) writeEvent->setPriority(priority);

  connectCBEvent->setPriority(priority);
  writeCBEvent->setPriority(priority);
  readCBEvent->setPriority(priority);
  errorCBEvent->setPriority(priority);
}


void BufferEvent::setTimeouts(unsigned read, unsigned write) {
  LOG_DEBUG(4, __func__ << '(' << read << ", " << write << ')');
  readTimeout = read;
  writeTimeout = write;
}


#ifdef HAVE_OPENSSL
cb::SSL BufferEvent::getSSL() const {return ssl;}
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
  for (unsigned i = 0; i < sslErrors.size(); i++) {
    if (!errors.empty()) errors += ", ";
    errors += SSL::getErrorStr(sslErrors[i]);
  }
#endif

  sslErrors.clear();

  return errors;
}


void BufferEvent::setRead(bool enabled) {
  if (enableRead == enabled) return;
  enableRead = enabled;

  if (state == STATE_SSL_READY || state == STATE_SOCK_READY)
    updateEvents();
}


string BufferEvent::getEventsString(short events) {
  vector<string> parts;

  if (events & BUFFEREVENT_READING)   parts.push_back("READING");
  if (events & BUFFEREVENT_WRITING)   parts.push_back("WRITING");
  if (events & BUFFEREVENT_EOF)       parts.push_back("EOF");
  if (events & BUFFEREVENT_ERROR)     parts.push_back("ERROR");
  if (events & BUFFEREVENT_TIMEOUT)   parts.push_back("TIMEOUT");

  return String::join(parts, "|");
}


void BufferEvent::close()  {
  if (getFD() < 0) return;

  readEvent.release();
  writeEvent.release();

  if (dnsReq.isSet()) {
    dnsReq->cancel();
    dnsReq.release();
  }

  socket.release();

  state = STATE_IDLE;
  enableRead = false;
}


void BufferEvent::connect(DNSBase &dns, const IPAddress &peer) {
  LOG_DEBUG(4, __func__ << "()");

  // Save peer port
  peerPort = peer.getPort();

  // Skip DNS lookup if we already have an IP
  if (peer.getIP()) {
    vector<IPAddress> ip;
    ip.push_back(peer);
    return dnsCB(0, ip);
  }

  // Must be before DNS call because it may callback immediately
  state = STATE_DNS_LOOKUP;
  updateEvents();

  // Start async DNS lookup
  if (dnsReq.isSet()) dnsReq->cancel();
  auto cb =
    [this] (int err, vector<IPAddress> &addrs, int ttl) {dnsCB(err, addrs);};
  dnsReq = dns.resolve(peer.getHost(), cb);
}


void BufferEvent::dnsCB(int err, const vector<IPAddress> &addrs) {
  LOG_DEBUG(4, __func__ << '(' << DNSRequest::getErrorStr(err) << ')');

  dnsReq.release();

  if (err || addrs.empty()) return scheduleErrorCB(BUFFEREVENT_ERROR);

  // Make sure we have an open socket
  if (socket.isNull()) socket = new Socket;
  if (!socket->isOpen()) {
    socket->open();
    setFD(socket->get());
  }

  try {
    IPAddress addr(addrs[0].getIP(), peerPort);

    socket->setBlocking(false);
    socket->connect(addr);

    state = STATE_SOCK_CONNECT;
    updateEvents();
    return;
  } CATCH_WARNING;

  scheduleErrorCB(BUFFEREVENT_ERROR);
}


void BufferEvent::errorCB() {
  LOG_DEBUG(4, __func__ << "(" << getEventsString(pendingErrorFlags) << ")");

  short what = pendingErrorFlags;
  int err = pendingError;

  pendingErrorFlags = 0;
  pendingError = 0;

  TRY_CATCH_ERROR(errorCB(what, err));
  logSSLErrors();
}


void BufferEvent::scheduleErrorCB(int flags, int err) {
  LOG_DEBUG(4, __func__ << "(" << getEventsString(flags) << ")");
  close();
  state = STATE_FAILED;
  pendingError = err ? err : SysError::get();
  pendingErrorFlags |= flags;
  errorCBEvent->activate();
}


void BufferEvent::scheduleReadCB() {
  if (inputBuffer.getLength() < minRead || state == STATE_FAILED) return;
  minRead = 0;
  readCBEvent->activate();
}


void BufferEvent::outputBufferCB(int added, int deleted, int orig) {
  LOG_DEBUG(4, __func__
            << "(" << added << ", " << deleted << ", " << orig << ")");
  if (added) updateEvents();
}


void BufferEvent::sockRead() {
  LOG_DEBUG(4, __func__ << "()");

  int ret = inputBuffer.read(getFD(), 1e6);

  if (0 < ret) return scheduleReadCB(); // Read callback

  int err = SysError::get();
  int flags = BUFFEREVENT_READING;

  if (!ret) flags |= BUFFEREVENT_EOF;
  else {
    if (ERR_RW_RETRIABLE(err)) return;
    flags |= BUFFEREVENT_ERROR;
  }

  scheduleErrorCB(flags);
}


void BufferEvent::sockConnect() {
  LOG_DEBUG(4, __func__ << "()");

  int err = 0;
  unsigned elen = sizeof(err);

  if (getsockopt(getFD(), SOL_SOCKET, SO_ERROR, (void *)&err, &elen) < 0)
    return scheduleErrorCB(BUFFEREVENT_ERROR);

  if (err) {
    if (ERR_CONNECT_RETRIABLE(err)) return;
    return scheduleErrorCB(BUFFEREVENT_ERROR, err);
  }

  connectCBEvent->activate();
  state = ssl ? STATE_SSL_HANDSHAKE : STATE_SOCK_READY;
}


void BufferEvent::sockWrite() {
  LOG_DEBUG(4, __func__ << "()");

  int ret = outputBuffer.write(getFD(), 1e6);

  if (ret <= 0) {
    int flags = BUFFEREVENT_WRITING;

    // TODO 0 on write doesn't actually indicate EOF, ECONNRESET may be better
    if (!ret) flags |= BUFFEREVENT_EOF;
    else {
      if (ERR_RW_RETRIABLE(SysError::get())) return;
      flags |= BUFFEREVENT_ERROR;
    }

    return scheduleErrorCB(flags);
  }

  // Invoke the user callback if buffer drained
  if (!outputBuffer.getLength()) writeCBEvent->activate();
}


void BufferEvent::sockReadCB(unsigned events) {
  LOG_DEBUG(4, __func__ << "(" << Event::getEventsString(events) << ")");

  SmartPointer<BufferEvent> self = this; // Don't deallocate during callback

  if (events == EVENT_TIMEOUT)
    return scheduleErrorCB(BUFFEREVENT_READING | BUFFEREVENT_TIMEOUT);

  switch (state) {
  case STATE_FAILED:
  case STATE_IDLE:
  case STATE_DNS_LOOKUP:
  case STATE_SOCK_CONNECT:
    LOG_WARNING(__func__ << "() Unexpected state " << state);
    return;

  case STATE_SOCK_READY:     sockRead();      break;
  case STATE_SSL_HANDSHAKE:  sslHandshake();  break;
  case STATE_SSL_READY:
    if (sslWant == SSL_ERROR_WANT_WRITE) sslWrite();
    else sslRead();
    break;
  }

  updateEvents();
}


void BufferEvent::sockWriteCB(unsigned events) {
  LOG_DEBUG(4, __func__ << "(" << Event::getEventsString(events) << ")");

  SmartPointer<BufferEvent> self = this; // Don't deallocate during callback

  if (events == EVENT_TIMEOUT)
    return scheduleErrorCB(BUFFEREVENT_WRITING | BUFFEREVENT_TIMEOUT);

  switch (state) {
  case STATE_FAILED:
  case STATE_IDLE:
  case STATE_DNS_LOOKUP:
    LOG_WARNING(__func__ << "() Unexpected state " << state);
    return;

  case STATE_SOCK_CONNECT:   sockConnect();  break;
  case STATE_SOCK_READY:     sockWrite();    break;
  case STATE_SSL_HANDSHAKE:  sslHandshake(); break;
  case STATE_SSL_READY:
    if (sslWant == SSL_ERROR_WANT_READ) sslRead();
    else sslWrite();
    break;
  }

  updateEvents();
}


void BufferEvent::sslClosed(unsigned when, int code, int ret) {
  LOG_DEBUG(4, __func__ << "(" << when << ", " << code << ", " << ret << ")");

  unsigned event = BUFFEREVENT_ERROR;

  switch (code) {
  case SSL_ERROR_ZERO_RETURN: event = BUFFEREVENT_EOF; break;
  case SSL_ERROR_SYSCALL: // IO error; possible dirty shutdown
    if (!ret && !SSL::peekError()) event = BUFFEREVENT_EOF;
    break;

  case SSL_ERROR_SSL: break; // Protocol error
  default:
    LOG_ERROR("BUG: Unexpected OpenSSL error code " << code);
    break;
  }

  // Save SSL errors
  unsigned err;
  while ((err = SSL::getError())) sslErrors.push_back(err);

  // when is BUFFEREVENT_{READING|WRITING}
  scheduleErrorCB(event | when);
}


void BufferEvent::sslError(unsigned event, int ret) {
  int err = SSL_get_error(ssl, ret);

  LOG_DEBUG(4, __func__ << "(" << getEventsString(event) << ", " << ret
            << ") err=" << err);

  switch (err) {
  case SSL_ERROR_WANT_READ:
    LOG_DEBUG(4, "SSL wants read");
    sslWant = SSL_ERROR_WANT_READ;
    break;

  case SSL_ERROR_WANT_WRITE:
    LOG_DEBUG(4, "SSL wants write");
    sslWant = SSL_ERROR_WANT_WRITE;
    break;

  default:
    LOG_ERROR(SSL::getErrorStr(SSL::peekError()));
    sslClosed(event, err, ret);
    break;
  }
}


void BufferEvent::sslRead() {
  LOG_DEBUG(4, __func__ << "()");

  sslWant = 0;
  size_t bytesRead = 0;
  size_t bytes = 0 < sslLastRead ? sslLastRead : 1e6;
  vector<iovec> space;

  while (bytes) {
    if (space.empty()) {
      space.resize(2);
      inputBuffer.reserve(bytes, space);
      if (space.empty()) return scheduleErrorCB(BUFFEREVENT_ERROR);
    }

    sslLastRead = min(bytes, space[0].iov_len);
    int ret = SSL_read(ssl, space[0].iov_base, sslLastRead);

    LOG_DEBUG(4, __func__ << "SSL try read " << sslLastRead << " ret=" << ret);

    if (0 < ret) {
#ifdef VALGRIND_MAKE_MEM_DEFINED
      (void)VALGRIND_MAKE_MEM_DEFINED(space[0].iov_base, ret);
#endif

      LOG_DEBUG(4, "SSL read " << ret);
      sslLastRead = 0;
      bytesRead += ret;
      bytes -= ret;
      space[0].iov_len = ret;
      inputBuffer.commit(space[0]);
      space.erase(space.begin());
      if (!bytes) bytes = SSL_pending(ssl);

    } else {
      if (!bytesRead) sslError(BUFFEREVENT_READING, ret);
      break; // Exit loop
    }
  }

  if (bytesRead) scheduleReadCB();
}


void BufferEvent::sslWrite() {
  LOG_DEBUG(4, __func__ << "()");

  sslWant = 0;
  size_t bytes = 0 < sslLastWrite ? sslLastWrite : 1e6;
  size_t bytesWritten = 0;
  vector<iovec> space(8);

  outputBuffer.peek(bytes, space);

  for (unsigned i = 0; i < space.size(); i++) {
    // Skip 0 length buffer, SSL_write() will return 0
    if (!space[i].iov_len) continue;

    sslLastWrite = min(bytes, space[i].iov_len);
    int ret = SSL_write(ssl, space[i].iov_base, sslLastWrite);

    if (0 < ret) {
      bytesWritten += ret;
      sslLastWrite = 0;

    } else {
      sslError(BUFFEREVENT_WRITING, ret);
      break; // Exit loop
    }
  }

  if (bytesWritten) {
    outputBuffer.drain(bytesWritten);
    if (!outputBuffer.getLength() && !errorCBEvent->isPending())
      writeCBEvent->activate();
  }
}


void BufferEvent::sslHandshake() {
  LOG_DEBUG(4, __func__ << "()");

  sslWant = 0;
  int ret = SSL_do_handshake(ssl);

  if (ret == 1) {
    LOG_DEBUG(4, "SSL Handshake complete");
    state = STATE_SSL_READY;

  } else sslError(BUFFEREVENT_READING, ret);
}


socket_t BufferEvent::getFD() const {
  return socket.isNull() ? -1 : socket->get();
}


void BufferEvent::setFD(socket_t fd) {
  int priority = getPriority();

  close();

  readEvent  = newEvent(fd, EVENT_READ, priority, &BufferEvent::sockReadCB);
  writeEvent = newEvent(fd, EVENT_WRITE, priority, &BufferEvent::sockWriteCB);

  if (0 <= priority) {
    readEvent->setPriority(priority);
    writeEvent->setPriority(priority);
  }

  if (ssl) {
    BIO *bio = BIO_new_socket(fd, 0);
    SSL_set_bio(ssl, bio, bio);
  }
}


void BufferEvent::enableEvents(unsigned events) {
  if ((events & EVENT_READ) && readEvent.isSet() && !readEvent->isPending())
    readEvent->add(readTimeout);

  if ((events & EVENT_WRITE) && writeEvent.isSet() && !writeEvent->isPending())
    writeEvent->add(writeTimeout);
}


void BufferEvent::disableEvents(unsigned events) {
  if ((events & EVENT_READ) && readEvent.isSet()) readEvent->del();
  if ((events & EVENT_WRITE) && writeEvent.isSet()) writeEvent->del();
}


void BufferEvent::updateEventsSSLWant() {
  switch (sslWant) {
  case SSL_ERROR_WANT_READ:
    enableEvents(EVENT_READ);
    disableEvents(EVENT_WRITE);
    break;

  case SSL_ERROR_WANT_WRITE:
    disableEvents(EVENT_READ);
    enableEvents(EVENT_WRITE);
    break;
  }
}


void BufferEvent::updateEvents() {
  switch (state) {
  case STATE_FAILED:        disableEvents();           break;
  case STATE_IDLE:                                     break;
  case STATE_DNS_LOOKUP:    disableEvents();           break;
  case STATE_SOCK_CONNECT:  enableEvents(EVENT_WRITE); break;

  case STATE_SSL_HANDSHAKE:
    if (sslWant) updateEventsSSLWant();
    else enableEvents();
    break;

  case STATE_SSL_READY:
    if (sslWant) {updateEventsSSLWant(); break;}
    // Fall through

  case STATE_SOCK_READY:
    if (outputBuffer.getLength()) enableEvents(EVENT_WRITE);
    else disableEvents(EVENT_WRITE);

    if (enableRead) enableEvents(EVENT_READ);
    else disableEvents(EVENT_READ);
    break;
  }

  bool readPending = readEvent.isSet() && readEvent->isPending();
  bool writePending = writeEvent.isSet() && writeEvent->isPending();
  LOG_DEBUG(4, __func__ << "() state=" << state << " read=" << readPending
            << " write=" << writePending);
}


SmartPointer<Event::Event>
BufferEvent::newEvent(socket_t fd, unsigned event, int priority,
                      void (BufferEvent::*member)(unsigned)) {
  auto cb =
    [this, member] (Event &e, int fd, unsigned events) {
      (this->*member)(events);
    };

  auto e = base.newEvent(fd, cb, event | EVENT_FINALIZE | EVENT_NO_SELF_REF);

  if (0 <= priority) e->setPriority(priority);

  return e;
}


SmartPointer<Event::Event>
BufferEvent::newEvent(void (BufferEvent::*member)()) {
  auto cb =
    [this, member] () {
      SmartPointer<BufferEvent> self = this; // Don't deallocate during callback
      (this->*member)();
    };

  auto e = base.newEvent(cb, EVENT_PERSIST | EVENT_NO_SELF_REF);
  return e;
}
