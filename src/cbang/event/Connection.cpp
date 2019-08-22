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

#include "Connection.h"
#include "Base.h"
#include "DNSBase.h"
#include "Request.h"
#include "HTTP.h"
#include "Event.h"
#include "Websocket.h"

#include <cbang/Exception.h>
#include <cbang/Catch.h>

#include <cbang/net/IPAddress.h>

#include <cbang/time/Timer.h>
#include <cbang/log/Logger.h>
#include <cbang/socket/Socket.h>
#include <cbang/os/SysError.h>

#ifdef HAVE_OPENSSL
#include <cbang/openssl/SSLContext.h>
#else
namespace cb {class SSLContext {};}
#endif

#ifdef _WIN32
#define ERRNO_CONNECT_REFUSED WSAECONNREFUSED
#else
#define ERRNO_CONNECT_REFUSED ECONNREFUSED
#endif

using namespace std;
using namespace cb;
using namespace cb::Event;


#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX << "CON" << getID() << ':'


Connection::Connection(Base &base, bool incoming, const IPAddress &peer,
                       const SmartPointer<Socket> &socket,
                       const SmartPointer<SSLContext> &sslCtx) :
  BufferEvent(base, incoming, socket, sslCtx), base(base),
  state(incoming ? STATE_READING_FIRSTLINE : STATE_DISCONNECTED),
  incoming(incoming), peer(peer), startTime(Timer::now()), sslCtx(sslCtx) {

  LOG_DEBUG(4, "created " << getStateString(state));
}


Connection::~Connection() {
  if (retryEvent.isSet()) {
    TRY_CATCH_ERROR(retryEvent->del());
    retryEvent.release();
  }

  LOG_DEBUG(4, "destroyed " << getStateString(state));
}


bool Connection::isConnected() const {
  return state != STATE_DISCONNECTED && state != STATE_CONNECTING;
}


const SmartPointer<Request> &Connection::getRequest() const {
  if (requests.empty()) THROW("No request");
  return requests.front();
}


void Connection::checkActiveRequest(Request &req) const {
  if (getRequest() != &req) THROW("Not the active request");
}


bool Connection::isWebsocket() const {
  return requests.size() && dynamic_cast<Websocket *>(requests.front().get());
}


Websocket &Connection::getWebsocket() const {
  return *getRequest().cast<Websocket>();
}


void Connection::sendServiceUnavailable() {
  LOG_DEBUG(4, __func__ << "()");

  if (requests.size()) THROW("Not a new Connection");
  push(new Request);
  getRequest()->sendError(HTTP_SERVICE_UNAVAILABLE);
}


void Connection::makeRequest(Request &req) {
  LOG_DEBUG(4, __func__ << "()");

  if (req.isReplying()) THROW("Request already replying");

  bool first = requests.empty();
  push(&req);

  // If this is the active request dispatch it
  if (first) dispatch();
}


void Connection::acceptRequest() {
  LOG_DEBUG(4, __func__ << "()");

  if (requests.size()) THROW("Not a new Connection");
  startRead();
}


void Connection::cancelRequest(Request &req) {
  LOG_DEBUG(4, __func__ << "()");

  if (getRequest() == &req) fail(CONN_ERR_REQUEST_CANCEL);
  else requests.remove(&req);
}


void Connection::write(Request &req, const Buffer &buf) {
  LOG_DEBUG(4, __func__ << "() bytes=" << buf.getLength());
  checkActiveRequest(req);

  getOutput().add(buf);

  detectClose = false; // Stop detect close

  // Don't change state or disable read if active Websocket
  if (!req.isWebsocket()) {
    setRead(false);
    setState(STATE_WRITING);
  }
}


const char *Connection::getStateString(state_t state) {
  switch (state) {
  case STATE_DISCONNECTED:      return "DISCONNECTED";
  case STATE_CONNECTING:        return "CONNECTING";
  case STATE_IDLE:              return "IDLE";
  case STATE_READING_FIRSTLINE: return "READING_FIRSTLINE";
  case STATE_READING_HEADERS:   return "READING_HEADERS";
  case STATE_READING_BODY:      return "READING_BODY";
  case STATE_READING_TRAILER:   return "READING_TRAILER";
  case STATE_WEBSOCK_HEADER:    return "WEBSOCK_HEADER";
  case STATE_WEBSOCK_BODY:      return "WEBSOCK_BODY";
  case STATE_WRITING:           return "WRITING";
  }

  return "INVALID";
}


void Connection::setState(state_t state) {
  if (state == this->state) return;
  LOG_DEBUG(4, getStateString(this->state) << " -> " << getStateString(state));
  this->state = state;
}


void Connection::push(const SmartPointer<Request> &req) {
  if (req->hasConnection()) THROW("Request already associated with Connection");
  req->setConnection(this);
  requests.push_back(req);
}


SmartPointer<Request> Connection::pop() {
  if (requests.empty()) THROW(__func__ << "() No requests");
  auto req = getRequest();
  requests.pop_front();
  TRY_CATCH_ERROR(req->onComplete());
  return req;
}


void Connection::free(ConnectionError error) {
  LOG_DEBUG(4, __func__ << '(' << error << ')');

  reset();

  // Remove request and callback
  while (!requests.empty()) {
    auto req = pop();
    if (!incoming) TRY_CATCH_ERROR(req->onResponse(error));
  }

  // Remove from HTTP
  if (http.isSet()) http->remove(*this);
}


void Connection::fail(ConnectionError error) {
  LOG_DEBUG(4, __func__ << '(' << error << ')');

  if (incoming) free(error);
  else {
    reset();
    TRY_CATCH_ERROR(pop()->onResponse(error));
    if (requests.size()) connect(); // Next request
    else free(error);
  }
}


void Connection::reset() {
  LOG_DEBUG(4, __func__ << "()");

  close();

  // Clear any buffered data
  getOutput().clear();
  getInput().clear();

  setState(STATE_DISCONNECTED);
}


void Connection::dispatch() {
  LOG_DEBUG(4, __func__ << "()");

  if (incoming) THROW("Expected outgoing Connection");
  if (!isConnected()) return connect();
  getRequest()->write();
}


void Connection::done() {
  LOG_DEBUG(4, __func__ << "()");

  // Incoming connection just processed request; respond
  // Outgoing connection just processed response; idle or close

  setRead(false); // Done reading

  auto req = getRequest();

  if (incoming) {
    setState(STATE_WRITING);                  // Start reply
    TRY_CATCH_ERROR(return req->onRequest()); // Callback
    fail(CONN_ERR_EXCEPTION);                 // Error on exception

  } else {
    // Idle or close the connection
    setState(STATE_IDLE);

    // Remove request
    pop();

    try {
      // Callback
      req->onResponse(CONN_ERR_OK);

      // Close connection if needed
      bool needsClose = req->needsClose();
      if (needsClose) reset();

      // If there are more requests start the next one
      if (!requests.empty()) dispatch();
      else if (!needsClose) {
        // Persistent Connection, detect if other side closes
        detectClose = true;
        setRead(true);

      } else free(CONN_ERR_OK); // Free if last request and not closing

      return;
    } CATCH_ERROR;

    free(CONN_ERR_EXCEPTION);
  }
}


void Connection::retry() {
  LOG_DEBUG(4, __func__ << "(" << retries << ")");

  reset();

  if (!maxRetries || retries < maxRetries) {
    if (retryEvent.isNull())
      retryEvent = base.newEvent(this, &Connection::connect, 0);

    if (!retries++) retryTimeout = 2;
    else retryTimeout *= 2; // Backoff

    retryEvent->add(retryTimeout);
    return;
  }

  // No more retries, fail all requests
  free(CONN_ERR_CONNECT);
}


void Connection::connect() {
  LOG_DEBUG(4, __func__ << "()");

  if (incoming) THROW("Cannot call connect() on incoming Connection");
  if (state == STATE_CONNECTING) return;
  state_t oldState = state;

  try {
    reset();

    // Open and bind new socket
    SmartPointer<Socket> socket = new Socket;
    if (bind.getIP()) socket->bind(bind);
    else socket->open();
    BufferEvent::setSocket(socket);

    setTimeouts(readTimeout, connectTimeout);
    setState(STATE_CONNECTING);

    BufferEvent::connect(getDNS(), peer);
    return;

  } CATCH_ERROR;

  setState(oldState);
  retry();
}


void Connection::websockClose(WebsockStatus status, const string &msg) {
  getWebsocket().close(status, msg);
}


void Connection::websockReadHeader() {
  LOG_DEBUG(4, __func__ << "()");

  setRead(true);
  setState(STATE_WEBSOCK_HEADER);

  if (getWebsocket().readHeader(getInput())) websockReadBody();
}


void Connection::websockReadBody() {
  setState(STATE_WEBSOCK_BODY);

  auto &ws = getWebsocket();

  if (ws.readBody(getInput())) {
    if (ws.isActive()) setState(STATE_WEBSOCK_HEADER);
    else setState(STATE_WRITING); // Writing close
  }
}


void Connection::startRead() {
  LOG_DEBUG(4, __func__ << "()");

  // Start reading response
  setRead(true);
  setState(STATE_READING_FIRSTLINE);
}


void Connection::newRequest(const string &line) {
  LOG_DEBUG(4, __func__ << "()");

  vector<string> parts;
  String::tokenize(line, parts, " ");

  if (parts.size() != 3) THROW("Invalid HTTP request line: " << line);

  RequestMethod method = RequestMethod::parse(parts[0]);
  URI uri = parts[1];
  Version version = Request::parseHTTPVersion(parts[2]);

  push(http->createRequest(*this, method, uri, version));
}


void Connection::readFirstLine() {
  LOG_DEBUG(4, __func__ << "()");

  try {
    string line = getInput().readLine(maxHeaderSize);
    if (line.empty()) return; // Need more data

    if (maxHeaderSize && maxHeaderSize < line.length())
      return getRequest()->sendError(HTTP_BAD_REQUEST, "Header too long");

    headerSize += line.length() + 2;

    if (incoming) newRequest(line);
    else getRequest()->parseResponseLine(line);

    readHeader();

  } catch (const Exception &e) {
    LOG_ERROR(e.getMessages());
    if (incoming) getRequest()->sendError(HTTP_BAD_REQUEST, e);
    else fail(CONN_ERR_EXCEPTION);
  }
}


bool Connection::tryReadHeader() {
  try {
    unsigned maxSize = maxHeaderSize ? maxHeaderSize - headerSize : 0;
    unsigned bytes = getInput().getLength();
    bool done = getRequest()->getInputHeaders().parse(getInput(), maxSize);

    headerSize += bytes - getInput().getLength();

    return done;

  } catch (const Exception &e) {
    LOG_ERROR(e.getMessages());
    if (incoming) getRequest()->sendError(HTTP_BAD_REQUEST, e);
    else fail(CONN_ERR_EXCEPTION);
  }

  return false;
}


void Connection::headersCallback() {
  TRY_CATCH_ERROR(return getRequest()->onHeaders());
  fail(CONN_ERR_EXCEPTION);
}


void Connection::readHeader() {
  LOG_DEBUG(4, __func__ << "()");

  setState(STATE_READING_HEADERS);

  if (!tryReadHeader()) return;

  // Request may be canceled based on headers
  headersCallback();
  if (state != STATE_READING_HEADERS) return;

  auto req = getRequest();

  // Done reading headers
  if (incoming) {
    // Handle protocol upgrades
    if (req->inHas("Upgrade")) {
      string upgrade = String::toLower(req->inFind("Upgrade"));

      if (upgrade == "websocket") {
        Websocket *websock = dynamic_cast<Websocket *>(req.get());
        if (websock && websock->upgrade()) {
          TRY_CATCH_ERROR(websock->onOpen(); return);
          websock->close(WS_STATUS_UNEXPECTED, "Exception");
          return;
        }
      }

      return req->sendError(HTTP_BAD_REQUEST, "Cannot upgrade");
    }

    getBody();

  } else {
    // Start over on 100 Continue
    if (req->getResponseCode() == 100) return startRead();

    if (req->mustHaveBody()) getBody();
    else done();
  }
}


void Connection::getBody() {
  LOG_DEBUG(4, __func__ << "()");

  // If this is a request without a body, then we are done
  if (incoming && !getRequest()->mayHaveBody()) return done();

  setState(STATE_READING_BODY);
  bytesToRead = -1;
  bodySize = 0;
  contentLength = -1;
  auto req = getRequest();

  string xferEnc = String::toLower(req->inFind("Transfer-Encoding"));
  if (xferEnc == "chunked") chunkedRequest = true;
  else {
    string contentLength = req->inFind("Content-Length");

    if (contentLength.empty()) {
      // Check for Connection: close
      string connection = String::toLower(req->inFind("Connection"));

      if (connection != "close") {
        // Bad combination, cannot tell when communication should end
        LOG_ERROR("No Content-Length but peer wants to keep connection open");
        return req->sendError(HTTP_BAD_REQUEST,
                             "Need Content-Length or Connection: close");
      }

      // Incoming non-chunked request /wo Content-Length has no body
      if (incoming) return done();

    } else {
      try {
        bytesToRead = String::parseU32(contentLength);
        this->contentLength = bytesToRead;
      } catch (const Exception &e) {
        return req->sendError(HTTP_BAD_REQUEST, "Invalid Content-Length");
      }

      if (maxBodySize && maxBodySize < (uint64_t)bytesToRead)
        return req->sendError(HTTP_REQUEST_ENTITY_TOO_LARGE);

      // Allocate space
      req->getInputBuffer().expand(bytesToRead);
    }
  }

  // 100 HTTP continue
  auto &version = req->getVersion();
  if (incoming && Version(1, 1) <= version && !getInput().getLength()) {
    string expect = String::toLower(req->inFind("Expect"));

    if (!expect.empty()) {
      if (expect == "100-continue")
        try {
          if (req->onContinue()) {
            getOutput()
              .add("HTTP/" + version.toString() + " 100 Continue\r\n\r\n");
            return;
          }
        } CATCH_ERROR;

      req->sendError(HTTP_EXPECTATION_FAILED);
      return;
    }
  }

  readBody();
}


void Connection::readBody() {
  LOG_DEBUG(4, __func__ << "()");

  auto buf = getInput();
  auto req = getRequest();

  if (chunkedRequest) {
    while (buf.getLength()) {
      if (bytesToRead < 0) {
        // Read chunk size
        string size = buf.readLine(12);

        // Last chunk on a new line?
        if (size.empty()) continue;

        unsigned bytes = String::parseU32("0x" + size);

        bodySize += bytes;
        bytesToRead = bytes;

        if (maxBodySize && maxBodySize < bodySize)
          return req->sendError(HTTP_BAD_REQUEST, "Too long");

        if (!bytesToRead) {
          // Finished last chunk
          headerSize = 0;
          return readTrailer();
        }
      }

      // Wait for end of chunk
      if (0 < bytesToRead && buf.getLength() < bytesToRead) break;

      // Completed chunk
      buf.remove(req->getInputBuffer(), bytesToRead);
      bytesToRead = -1;
    }

  } else {
    unsigned bytes = buf.getLength();

    if (0 <= bytesToRead) {
      if (bytesToRead < bytes) bytes = bytesToRead;
      bytesToRead -= bytes;
    }

    bodySize += bytes;

    if (maxBodySize && maxBodySize < bodySize)
      return req->sendError(HTTP_BAD_REQUEST, "Too long");

    buf.remove(req->getInputBuffer(), bytes);
  }

  // Report progress
  TRY_CATCH_ERROR(req->onProgress(bodySize, contentLength));

  if (bytesToRead) {
    setRead(true); // Read more
    if (0 < bytesToRead) setMinRead(min((int64_t)1 << 14, bytesToRead));

  } else done();
}


void Connection::readTrailer() {
  LOG_DEBUG(4, __func__ << "()");
  setState(STATE_READING_TRAILER);

  if (tryReadHeader()) done();
}


void Connection::connectCB() {
  if (state != STATE_CONNECTING)
    THROW("Unexpected connect callback state=" << state);

  LOG_DEBUG(3, "Connected to " << peer);
  retries = 0; // Success
  setState(STATE_IDLE);

  // Set Read/Write timeouts
  setTimeouts(readTimeout, writeTimeout);

  // Start new requests
  if (!requests.empty()) dispatch();
}


void Connection::readCB() {
  LOG_DEBUG(4, __func__ << "() bytes=" << getInput().getLength());

  try {
    switch (state) {
    case STATE_DISCONNECTED:      break;
    case STATE_CONNECTING:        return;
    case STATE_IDLE:              return reset();
    case STATE_READING_FIRSTLINE: return readFirstLine();
    case STATE_READING_HEADERS:   return readHeader();
    case STATE_READING_BODY:      return readBody();
    case STATE_READING_TRAILER:   return readTrailer();
    case STATE_WEBSOCK_HEADER:    return websockReadHeader();
    case STATE_WEBSOCK_BODY:      return websockReadBody();
    case STATE_WRITING:           return;
    }

    THROW("Unexpected state " << state << " in read callback");
  } CATCH_ERROR;

  fail(CONN_ERR_UNKNOWN);
}


/// Invoked when data has been written
void Connection::writeCB() {
  LOG_DEBUG(4, __func__ << "() bytes=" << getOutput().getLength());

  try {
    switch (state) {
    case STATE_CONNECTING: return;
    case STATE_READING_BODY: // Handle Continue during body read
    case STATE_WEBSOCK_HEADER:
    case STATE_WEBSOCK_BODY:
    case STATE_WRITING:
      // NOTE, Output buffer must be empty if write low watermark zero
      if (getOutput().getLength()) return; // Still writing
      break;
    default: THROW("Unexpected state " << state << " in write callback");
    }

    if (state != STATE_WRITING) return;

    auto req = getRequest();

    if (incoming) {
      if (req->isChunked()) return; // Still writing
      if (req->isWebsocket()) return websockReadHeader();

      // Done
      pop();

      // Free connection if not persistent
      auto &version = req->getVersion();
      bool keepAlive = req->getInputHeaders().connectionKeepAlive();
      if ((version < Version(1, 1) && !keepAlive) || req->needsClose())
        return free(CONN_ERR_OK);
    }

    // Incoming: accept another request
    // Outgoing: all output written, read response
    startRead();

    return;
  } CATCH_ERROR;

  fail(CONN_ERR_UNKNOWN);
}


void Connection::errorCB(short what, int err) {
  LOG_DEBUG(5, __func__ << "(" << BufferEvent::getEventsString(what) << ") "
            << getStateString(state));

  switch (state) {
  case STATE_DISCONNECTED: return;

  case STATE_CONNECTING:
    LOG_DEBUG(3, "Connection failed for " << peer << " with "
              << BufferEvent::getEventsString(what)
              << ", System Error: " << SysError(err));

    if (err == ERRNO_CONNECT_REFUSED) return free(CONN_ERR_CONNECT);
    if (what & BUFFEREVENT_TIMEOUT) fail(CONN_ERR_TIMEOUT);
    else retry();

    return;

  case STATE_WEBSOCK_HEADER: case STATE_WEBSOCK_BODY: pop(); break;

  case STATE_READING_BODY:
    if (!chunkedRequest && bytesToRead < 0 &&
        what == (BUFFEREVENT_READING | BUFFEREVENT_EOF))
      return done(); // Benign EOF on read

  default: break;
  }

  // When in close detect mode, a read error means the other side closed
  if (detectClose) {
    detectClose = false;
    if (state != STATE_IDLE || incoming)
      LOG_ERROR(__func__ << "() Unexpected detect close state " << state);
    reset();

    // Free if no more requests
    if (requests.empty()) free(CONN_ERR_OK);
    return;
  }

  if (what & BUFFEREVENT_TIMEOUT) fail(CONN_ERR_TIMEOUT);
  else if (what & (BUFFEREVENT_EOF | BUFFEREVENT_ERROR)) fail(CONN_ERR_EOF);
  else fail(CONN_ERR_BUFFER_ERROR);
}
