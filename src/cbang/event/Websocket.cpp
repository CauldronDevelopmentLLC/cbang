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

#include "Websocket.h"
#include "Connection.h"

#include <cbang/Catch.h>
#include <cbang/net/Swab.h>
#include <cbang/openssl/Digest.h>
#include <cbang/util/Random.h>

using namespace std;
using namespace cb;
using namespace cb::Event;


void Websocket::send(const char *data, uint64_t length) {
  if (!active) return Request::send(data, length);

  const unsigned frameSize = 0xffff;

  for (uint64_t i = 0; length; i += frameSize) {
    uint64_t bytes = frameSize < length ? frameSize : length;
    length -= bytes;
    writeFrame(i ? WS_OP_CONTINUE : WS_OP_TEXT, !length, data + i, bytes);
  }

  msgSent++;
}


void Websocket::send(const string &s) {
  if (!active) return Request::send(s);
  send(s.c_str(), s.length());
}


void Websocket::close(WebsockStatus status, const std::string &msg) {
  LOG_DEBUG(4, __func__ << '(' << status << ", " << msg << ')');

  if (!active) return; // Already closed

  uint16_t data = hton16(status);
  writeFrame(WS_OP_CLOSE, true, &data, 2);

  active = false;
  TRY_CATCH_ERROR(onClose(status, msg));
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
    key = Digest::base64(key, "sha1");

    setVersion(Version(1, 1));
    outSet("Upgrade", "websocket");
    outSet("Connection", "upgrade");
    outSet("Sec-WebSocket-Accept", key);

    reply(HTTP_SWITCHING_PROTOCOLS);
    active = true; // Must be after above reply; See Connection::write()

    // TODO Timeout out connection

    return true;
  } CATCH_ERROR;

  return false;
}


bool Websocket::readHeader(Buffer &input) {
  if (input.getLength() < 2) return false; // Wait for more

  uint8_t header[14];
  input.copy((char *)header, 2);

  // Client must set mask bit
  bool mask = header[1] & 0x80;
  if (!mask) {
    close(WS_STATUS_PROTOCOL);
    return false;
  }

  // Compute header size
  uint8_t bytes = 6;
  uint8_t size = header[1] & 0x7f;
  if (size == 126) bytes = 8;
  if (size == 127) bytes = 14;

  // Read frame header
  if (input.getLength() < bytes) return false; // Wait for more
  input.remove((char *)header, bytes);

  // Compute frame size
  if (size == 126) bytesToRead = hton16(*(uint16_t *)&header[2]);
  else if (size == 127) bytesToRead = hton64(*(uint64_t *)&header[2]);
  else bytesToRead = size;
  if (bytesToRead & (1ULL << 63)) {
    close(WS_STATUS_PROTOCOL);
    return false;
  }

  // Check opcode
  uint8_t opcode = header[0] & 0xf;
  wsOpCode = (WebsockOpCode::enum_t)opcode;

  if (wsOpCode != WS_OP_CONTINUE) wsMsg.clear();

  switch (wsOpCode) {
  case WS_OP_TEXT:
  case WS_OP_BINARY:
  case WS_OP_CONTINUE: {
    // Check total message size
    auto maxBodySize = getConnection().getMaxBodySize();
    if (maxBodySize && maxBodySize < wsMsg.size() + bytesToRead) {
      close(WS_STATUS_TOO_BIG);
      return false;
    }

    // Copy mask
    memcpy(wsMask, &header[bytes - 4], 4);

    // Last part of message?
    wsFinish = 0x80 & header[0];
    break;
  }

  case WS_OP_CLOSE:
  case WS_OP_PING:
  case WS_OP_PONG:
    break; // Control codes

  default: {
    close(WS_STATUS_PROTOCOL);
    return false;
  }
  }

  return true;
}


bool Websocket::readBody(Buffer &input) {
  LOG_DEBUG(4, __func__ << "() bytesToRead=" << bytesToRead
            << " inBuf=" << input.getLength());

  if (input.getLength() < bytesToRead) return false; // Wait for more

  uint64_t offset = wsMsg.size();
  wsMsg.resize(offset + bytesToRead);
  input.remove(&wsMsg[offset], bytesToRead);

  // Demask
  for (uint64_t i = 0; i < bytesToRead; i++)
    wsMsg[offset +  i] ^= wsMask[i & 3];

  LOG_DEBUG(5, "Frame body\n"
            << String::hexdump(string(wsMsg.begin() + offset, wsMsg.end()))
            << '\n');

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
    WebsockStatus status = WS_STATUS_NONE;
    if (1 < bytesToRead)
      status = (WebsockStatus::enum_t)hton16(*(uint16_t *)wsMsg.data());

    // Send close response and close payload if any
    close(status, string(wsMsg.begin() + 2, wsMsg.end()));
    break;
  }

  case WS_OP_PING:
    // TODO Schedule pong to aggregate backlogged pings
    pongPayload = string(wsMsg.begin(), wsMsg.end());
    pong();
    break;

  case WS_OP_PONG:
    // TODO Clear ping timeout
    break;

  default: close(WS_STATUS_PROTOCOL); break;
  }

  return true;
}


void Websocket::onMessage(const char *data, uint64_t length) {
  if (cb) cb(data, length);
}



void Websocket::writeFrame(WebsockOpCode opcode, bool finish,
                           const void *data, uint64_t len) {
  LOG_DEBUG(4, __func__ << '(' << opcode << ", " << finish << ", " << len
            << ')');

  if (!active) THROW("Not active");

  uint8_t header[14];
  uint8_t bytes = 2;

  // Opcode
  header[0] = (finish ? 0x80 : 0) | opcode;

  // Format payload length
  if (len < 126) header[1] = len;

  else if (len <= 0xffff) {
    header[1] = 126;
    *(uint16_t *)&header[2] = hton16(len);
    bytes = 4;

  } else {
    header[1] = 127;
    *(uint64_t *)&header[2] = hton64(len);
    bytes = 10;
  }

  // Create mask
  bool maskData = !getConnection().isIncoming();
  if (maskData) {
    header[1] &= 1 << 7; // Set mask bit

    // Generate random mask
    Random::instance().bytes(header + bytes, 4);
    bytes += 4;
  }

  Buffer out;
  out.expand(len + bytes);
  out.add((char *)header, bytes);
  out.add((char *)data, len);

  // Mask data
  if (maskData) {
    uint8_t *ptr = (uint8_t *)out.pullup(len + bytes) + bytes;
    uint8_t *mask = &header[bytes - 4];

    for (uint64_t i = 0; i < len; i++)
      ptr[i] ^= mask[i & 3];
  }

  getConnection().write(*this, out);
}


void Websocket::pong() {
  writeFrame(WS_OP_PONG, true, pongPayload.data(), pongPayload.size());
  pongPayload.clear();
}


void Websocket::message(const char *data, uint64_t length) {
  msgReceived++;
  TRY_CATCH_ERROR(return onMessage(data, length));
  close(WS_STATUS_UNACCEPTABLE, "Message rejected");
}
