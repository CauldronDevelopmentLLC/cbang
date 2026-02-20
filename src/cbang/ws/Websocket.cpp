/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
                Copyright (c) 2003-2021, Cauldron Development LLC
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

#include "Websocket.h"

#include <cbang/Catch.h>
#include <cbang/net/Swab.h>
#include <cbang/net/Base64.h>
#include <cbang/util/Random.h>
#include <cbang/util/WeakCallback.h>
#include <cbang/http/Client.h>
#include <cbang/http/ConnOut.h>
#include <cbang/http/OutgoingRequest.h>

#ifdef HAVE_OPENSSL
#include <cbang/openssl/Digest.h>
#endif

#include <cstring> // memcpy()

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX "WS" << getID() << ':'


using namespace std;
using namespace cb;
using namespace cb::WS;


uint64_t Websocket::getID() const {
  return connection.isSet() ? connection->getID() : 0;
}


bool Websocket::isActive() const {
  return active && connection.isSet() && connection->isConnected();
}


void Websocket::connect(HTTP::Client &client, const URI &uri) {
  HTTP::Client::callback_t cb = [this] (HTTP::Request &req) {
    auto code  = req.getResponseCode();
    auto error = req.getConnectionError();

    if (error == CONN_ERR_OK && code == HTTP_SWITCHING_PROTOCOLS) {
      LOG_DEBUG(4, "Opened new Websocket: " << getID());
      start();

    } else {
      onClose(WS_STATUS_NONE,
        SSTR("Connection failed: error=" << error << " code=" << code));
      connection.release();
    }
  };

  auto req = SmartPtr(new HTTP::OutgoingRequest(
    connection, uri, HTTP_GET, WeakCall(this, cb)));

  char nonce[16];
  Random::instance().bytes(nonce, 16);

  req->outSet("Sec-WebSocket-Key",     Base64().encode(nonce, 16));
  req->outSet("Sec-WebSocket-Version", "13");
  req->outSet("Upgrade",               "websocket");
  req->outSet("Connection",            "upgrade");

  connection = client.send(req);
}


void Websocket::send(const char *data, unsigned length) {
  const unsigned frameSize = 0xffff;

  for (unsigned i = 0; length; i += frameSize) {
    unsigned bytes = frameSize < length ? frameSize : length;
    length -= bytes;
    writeFrame(i ? WS_OP_CONTINUE : WS_OP_TEXT, !length, data + i, bytes);
  }

  msgSent++;
}


void Websocket::send(const string &s) {send(s.data(), s.length());}


void Websocket::close(Status status, const string &msg) {
  LOG_DEBUG(4, CBANG_FUNC << '(' << status << ", " << msg << ')');

  pingEvent.release();
  pongEvent.release();

  if (isActive()) {
    uint8_t data[125];
    *(uint16_t *)data = hton16(status);
    unsigned len = msg.length() < 123 ? msg.length() : 123;
    memcpy(data + 2, msg.c_str(), len);
    writeFrame(WS_OP_CLOSE, true, data, 2 + len);
  }

  if (active) {
    active = false;
    TRY_CATCH_ERROR(onClose(status, msg));
    active = true; // shutdown() expects active = true
    shutdown();
  }
}


void Websocket::ping(const string &payload) {
  writeFrame(WS_OP_PING, true, payload.data(), payload.size());
}


void Websocket::upgrade(HTTP::Request &req) {
   if (String::toLower(req.inFind("Upgrade")) != "websocket")
     THROWX("Expected websocket request", HTTP_BAD_REQUEST);

  // Check if websocket upgrade is allowed
  bool upgrade = req.getInputHeaders().keyContains("Connection", "upgrade");
  string key = req.inFind("Sec-WebSocket-Key");

  if (!upgrade || key.empty() || req.getVersion() < Version(1, 1))
    THROWX("Invalid websocket request", HTTP_BAD_REQUEST);

  // Send handshake
  key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
#ifdef HAVE_OPENSSL
  key = Digest::base64(key, "sha1");
#else
  THROWX("Cannot open Websocket, C! not built with openssl support",
    HTTP_NOT_IMPLEMENTED);
#endif

  // Respond
  req.setVersion(Version(1, 1));
  req.outSet("Upgrade", "websocket");
  req.outSet("Connection", "upgrade");
  req.outSet("Sec-WebSocket-Accept", key);
  req.reply(HTTP_SWITCHING_PROTOCOLS);

  connection = req.getConnection();
  start();
}


void Websocket::readHeader() {
  Event::Transfer::cb_t cb =
    [this] (bool success) {
      if (!success)
        return close(WS_STATUS_PROTOCOL, "Failed to read header start");

      uint8_t header[2];
      input.copy((char *)header, 2);

      // Client must set mask bit
      bool mask = header[1] & (1 << 7);
      if (mask != connection->isIncoming())
        return close(WS_STATUS_PROTOCOL, "Header mask mismatch");

      // Compute header size
      uint8_t bytes = mask ? 6 : 2;
      uint8_t size = header[1] & 0x7f;
      if (size == 126) bytes += 2;
      if (size == 127) bytes += 8;

      Event::Transfer::cb_t cb =
        [this, mask, bytes, size] (bool success) {
          if (!success)
            return close(WS_STATUS_PROTOCOL, "Failed to read header end");

          uint8_t header[14];
          input.remove((char *)header, bytes);

          // Compute frame size
          if      (size == 126) bytesToRead = hton16((uint16_t &)header[2]);
          else if (size == 127) bytesToRead = hton64((uint64_t &)header[2]);
          else                  bytesToRead = size;
          if (bytesToRead & (1ULL << 63))
            return close(WS_STATUS_PROTOCOL, "Invalid frame size");

          // Check opcode
          wsOpCode = (OpCode::enum_t)(header[0] & 0xf);

          LOG_DEBUG(4, CBANG_FUNC << "() opcode=" << wsOpCode
                    << " bytes=" << bytesToRead);

          if (wsOpCode != WS_OP_CONTINUE) wsMsg.clear();

          // Check total message size
          auto msgSize = wsMsg.size() + bytesToRead;
          if (maxMessageSize && maxMessageSize < msgSize)
            return close(WS_STATUS_TOO_BIG,
              SSTR("Message size " << msgSize << ">" << maxMessageSize));

          // Copy mask
          if (mask) memcpy(wsMask, &header[bytes - 4], 4);

          // Last part of message?
          wsFinish = header[0] & (1 << 7);

          // Control frames must not be fragmented
          if ((wsOpCode & 8) && !wsFinish)
            return close(WS_STATUS_PROTOCOL, "Fragmented control frame");

          readBody();
        };

      // Read rest of header
      getConnection()->read(WeakCall(this, cb), input, bytes);
    };

  // Read first part of header
  getConnection()->read(WeakCall(this, cb), input, 2);
}


void Websocket::readBody() {
  Event::Transfer::cb_t cb = [this] (bool success) {
    if (!success) return close(WS_STATUS_PROTOCOL, "Failed to ready body");

    if (bytesToRead) {
      uint64_t offset = wsMsg.size();
      wsMsg.resize(offset + bytesToRead);
      input.remove(&wsMsg[offset], bytesToRead);

      // Demask client messages
      if (connection->isIncoming())
        for (uint64_t i = 0; i < bytesToRead; i++)
          wsMsg[offset +  i] ^= wsMask[i & 3];

      LOG_DEBUG(5, "Frame body\n"
                << String::hexdump(string(wsMsg.begin() + offset, wsMsg.end()))
                << '\n');
    }

    switch (wsOpCode) {
    case WS_OP_CONTINUE:
    case WS_OP_TEXT:
    case WS_OP_BINARY:
      if (wsFinish) {
        message(wsMsg.data(), wsMsg.size());
        wsMsg.clear();
      }
      break;

    case WS_OP_CLOSE: {
      // Get close status
      Status status = WS_STATUS_NONE;
      if (1 < bytesToRead)
        status = (Status::enum_t)hton16(*(uint16_t *)wsMsg.data());

      // Send close response and close payload if any
      string payload;
      if (2 < wsMsg.size()) payload = string(wsMsg.begin() + 2, wsMsg.end());
      return close(status, payload);
    }

    case WS_OP_PING: onPing(string(wsMsg.begin(), wsMsg.end())); break;
    case WS_OP_PONG: onPong(string(wsMsg.begin(), wsMsg.end())); break;

    default: return close(WS_STATUS_PROTOCOL, "Invalid opcode");
    }

    // Read next message
    if (isActive()) readHeader();
  };

  getConnection()->read(WeakCall(this, cb), input, bytesToRead);
}


void Websocket::onPing(const string &payload) {
  LOG_DEBUG(4, CBANG_FUNC << "() payload=" << String::escapeC(payload));

  // Defer pong to aggregate any backlog of pings
  pongPayload = payload;
  schedulePong();
}


void Websocket::onPong(const string &payload) {
  LOG_DEBUG(4, CBANG_FUNC << "() payload=" << String::escapeC(payload));
  schedulePing();
}


void Websocket::writeFrame(
  OpCode opcode, bool finish, const void *data, uint64_t len) {
  LOG_DEBUG(4, CBANG_FUNC << '(' << opcode << ", " << finish << ", " << len
            << ')');

  if (!isActive()) {
    close(WS_STATUS_DIRTY_CLOSE, "Write attempted when inactive");
    THROW("Websocket not active");
  }

  uint8_t header[14];
  uint8_t bytes = 2;

  // Opcode
  header[0] = (finish ? (1 << 7) : 0) | opcode;

  // Format payload length
  if (len < 126) header[1] = len;

  else if (len <= 0xffff) {
    header[1] = 126;
    (uint16_t &)header[2] = hton16(len);
    bytes = 4;

  } else {
    header[1] = 127;
    (uint64_t &)header[2] = hton64(len);
    bytes = 10;
  }

  // Create mask
  bool mask = !connection->isIncoming();
  if (mask) {
    header[1] |= 1 << 7; // Set mask bit

    // Generate random mask
    Random::instance().bytes(header + bytes, 4);
    bytes += 4;
  }

  Event::Buffer out;
  out.expand(len + bytes);
  out.add((char *)header, bytes);
  out.add((char *)data, len);

  // Mask data
  if (mask) {
    uint8_t *ptr  = (uint8_t *)out.pullup(len + bytes) + bytes;
    uint8_t *mask = &header[bytes - 4];

    for (uint64_t i = 0; i < len; i++)
      ptr[i] ^= mask[i & 3];
  }

  Event::Transfer::cb_t cb =
    [this, opcode] (bool success) {
      // Close connection if write fails or this is a close op code
      if (!success || opcode == WS_OP_CLOSE) shutdown();
    };

  getConnection()->write(WeakCall(this, cb), out);
}


void Websocket::pong() {
  writeFrame(WS_OP_PONG, true, pongPayload.data(), pongPayload.size());
  pongPayload.clear();
}


void Websocket::schedulePong() {
  if (!active) return;

  if (pongEvent.isNull()) {
    auto cb = [this] () {pong();};
    pongEvent = getConnection()->getBase().newEvent(cb, 0);
  }

  if (!pongEvent->isPending()) pongEvent->add(5);
}


void Websocket::schedulePing() {
  if (!active) return;

  if (pingEvent.isNull()) {
    auto cb = [this] () {ping();};
    pingEvent = getConnection()->getBase().newEvent(cb);
  }

  double timeout = getConnection()->getReadTimeout();
  pingEvent->add(10 < timeout ? timeout / 2 : 5);
}


void Websocket::message(const char *data, uint64_t length) {
  msgReceived++;
  if (pingEvent.isSet() && pingEvent->isPending())
    schedulePing(); // Reschedule ping for later

  try {
    onMessage(data, length);

  } catch (const Exception &e) {
    string msg = "Websocket message rejected: " + e.getMessage();
    LOG_DEBUG(3, msg);
    LOG_DEBUG(4, e);
    close(WS_STATUS_UNACCEPTABLE, msg);
  }
}


void Websocket::start() {
  active = true;
  onOpen();
  readHeader();
  schedulePing();
}


void Websocket::shutdown() {
  if (connection.isSet()) connection->close();
  if (active) {
    active = false;
    TRY_CATCH_ERROR(onShutdown());
  }
}
