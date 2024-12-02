/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include <cbang/http/Conn.h>
#include <cbang/Catch.h>
#include <cbang/net/Swab.h>
#include <cbang/util/Random.h>
#include <cbang/util/WeakCallback.h>

#ifdef HAVE_OPENSSL
#include <cbang/openssl/Digest.h>
#else
#include <cbang/net/Base64.h>
#endif

#include <cstring> // memcpy()

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX "WS" << getID() << ':'


using namespace std;
using namespace cb;
using namespace cb::WS;


Websocket::Websocket(
  const SmartPointer<HTTP::Conn> &connection, const URI &uri,
  const Version &version) :
  Request(connection, HTTP_GET, uri, version) {
  if (version < Version(1, 1)) THROW("Invalid HTTP version for websocket");
}


bool Websocket::isActive() const {return active && isConnected();}


void Websocket::send(const char *data, unsigned length) {
  if (!active) return Request::send(data, length);

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
  }
}


void Websocket::ping(const string &payload) {
  writeFrame(WS_OP_PING, true, payload.data(), payload.size());
}


bool Websocket::upgrade() {
  // Check if websocket upgrade is allowed
  bool upgrade = getInputHeaders().keyContains("Connection", "upgrade");
  string key = inFind("Sec-WebSocket-Key");

  if (!upgrade || key.empty() || getVersion() < Version(1, 1)) return false;

  try {
    // Callback
    if (!onUpgrade()) return false;

    // Send handshake
    key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
#ifdef HAVE_OPENSSL
    key = Digest::base64(key, "sha1");
#else
    THROW("Cannot open Websocket, C! not built with openssl support");
#endif

    // Activate Websocket
    active = true;

    // Respond
    setVersion(Version(1, 1));
    outSet("Upgrade", "websocket");
    outSet("Connection", "upgrade");
    outSet("Sec-WebSocket-Accept", key);
    reply(HTTP_SWITCHING_PROTOCOLS);

    // Start connection
    onOpen();
    readHeader();
    schedulePing();

    return true;
  } CATCH_ERROR;

  close(WS_STATUS_UNEXPECTED, "Exception");
  return false;
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
      if (mask != isIncoming())
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
          auto maxBodySize = getConnection()->getMaxBodySize();
          auto msgSize = wsMsg.size() + bytesToRead;
          if (maxBodySize && maxBodySize < msgSize)
            return close(WS_STATUS_TOO_BIG,
              SSTR("Message size " << msgSize << ">" << maxBodySize));

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
      if (isIncoming())
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


void Websocket::onResponse(Event::ConnectionError error) {
  Request::onResponse(error);

  if (error == CONN_ERR_OK && getResponseCode() == HTTP_SWITCHING_PROTOCOLS) {
    LOG_DEBUG(4, "Opened new Websocket");
    active = true;
    onOpen();
    readHeader();
    schedulePing();

  } else onClose(WS_STATUS_NONE, SSTR("Connection failed: " << error));
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


void Websocket::writeRequest(Event::Buffer &buf) {
  char nonce[16];
  Random::instance().bytes(nonce, 16);

  outSet("Sec-WebSocket-Key",     Base64().encode(nonce, 16));
  outSet("Sec-WebSocket-Version", "13");
  outSet("Upgrade",               "websocket");
  outSet("Connection",            "upgrade");

  Request::writeRequest(buf);
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
  bool mask = !isIncoming();
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
      if (!success || opcode == WS_OP_CLOSE)
        getConnection()->close();
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
  if (pingEvent->isPending()) schedulePing(); // Reschedule ping for later

  try {
    onMessage(data, length);

  } catch (const Exception &e) {
    string msg = "Websocket message rejected: " + e.getMessage();
    LOG_DEBUG(3, msg);
    close(WS_STATUS_UNACCEPTABLE, msg);
  }
}
