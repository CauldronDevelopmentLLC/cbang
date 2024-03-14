/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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

#include "Nameserver.h"
#include "Base.h"
#include "Request.h"
#include "Result.h"

#include <cbang/Catch.h>
#include <cbang/log/Logger.h>
#include <cbang/event/Base.h>
#include <cbang/event/Event.h>
#include <cbang/net/Socket.h>
#include <cbang/net/Swab.h>
#include <cbang/time/Time.h>
#include <cbang/util/Random.h>

#include <cstring>

using namespace std;
using namespace cb;
using namespace cb::DNS;

#undef CBANG_LOG_PREFIX
#define CBANG_LOG_PREFIX "NS:" << addr << ':'

#define CLASS_INET 1


namespace {
  string parseName(uint8_t *packet, unsigned length, unsigned &offset) {
    unsigned i = offset;
    unsigned ptrCount = 0;
    char name[256];
    char *out = name;

    while (i < length) {
      uint8_t len = packet[i++];
      if (!len) break; // End of label

      if (len & 0xc0) { // Pointer
        if (i == length) THROW("Name pointer overflow in DNS response");
        unsigned ptr = ((len & 0x3f) << 8) + packet[i++];
        if (length <= ptr) THROW("Invalid name pointer in DNS response");
        if (!ptrCount) offset = i;
        if (length < ++ptrCount) THROW("Name pointer loop in DNS response");
        i = ptr;
        continue;
      }

      if (63 < len) THROW("Label too long in DNS response");

      unsigned end = (unsigned)(out - name) + len + (out != name ? 1 : 0) + 1;
      if (sizeof(name) <= end) THROW("Name too long in DNS response");

      if (out != name) *out++ = '.';
      memcpy(out, packet + i, len);
      out += len;
      i += len;
    }

    if (!ptrCount) offset = i;

    *out = 0;
    return name;
  }


  string randomizeCase(const std::string &s) {
    string r;
    unsigned bits = 0;
    uint32_t rand = 0;

    for (auto c: s)
      if (isalpha(c)) {
        if (!bits) {
          rand = Random::instance().rand<uint32_t>();
          bits = 32;
        }
        r.append(1, (char)((rand & (1 << --bits)) ? toupper(c) : c));

      } else r.append(1, c);

    return r;
  }
}


Nameserver::Nameserver(Base &base, const SockAddr &addr, bool system) :
  base(base), addr(addr), system(system), socket(new Socket) {
  if (!addr.getPort()) this->addr.setPort(53);
  start();
}


Nameserver::~Nameserver() {}


void Nameserver::start() {
  failures = 0;
  waiting  = false;

  socket->open(true, addr.isIPv6());
  socket->setBlocking(false);

  // Bind outgoing address if not loopback
  auto &bind = base.getBindAddress();
  if (!bind.isEmpty() && !addr.isLoopback()) socket->bind(bind);

  event = base.getEventBase().newEvent(
    socket->get(), this, &Nameserver::ready,
    EVENT_READ | EVENT_PERSIST | EVENT_NO_SELF_REF);
  event->add();
}


void Nameserver::stop() {
  for (auto &it: active) {
    auto &query = it.second;
    query.timeout->del();
    respond(query, new Result(DNS_ERR_SHUTDOWN), 0);
  }

  active.clear();
  event->del();
  socket->close();
}


bool Nameserver::transmit(Type type, const string &request) {
  if (255 < request.size()) THROW("DNS request too large");

  if (base.getMaxActive() <= active.size()) return false;

  // Choose an inactive random ID
  uint16_t id;
  while (true) {
    id = Random::instance().rand<uint16_t>();
    auto it = active.find(id);
    if (it == active.end()) break;
  }

  // Randomize request case
  Query query = {type, randomizeCase(request)};

  // Create buffer
  uint8_t buf[1024] = {0};
  ((uint16_t *)buf)[0] = hton16(id);
  ((uint16_t *)buf)[1] = hton16(0x100); // standard query with recursion
  ((uint16_t *)buf)[2] = hton16(1);     // one question

  vector<string> parts;
  String::tokenize(query.request, parts, ".");
  unsigned i = 12;
  for (unsigned j = 0; j < parts.size(); j++) {
    unsigned len = parts[j].size();
    if (63 < len) THROW("Invalid DNS request, name part too big");
    buf[i] = len;
    memcpy(buf + i + 1, parts[j].data(), len);
    i += 1 + len;
  }
  buf[i++] = 0;

  ((uint16_t *)(buf + i))[0] = hton16(type);
  ((uint16_t *)(buf + i))[1] = hton16(CLASS_INET);
  i += 4;

  // Send it
  try {
    if (socket->write(buf, i, 0, &addr) == i) {
      query.timeout =
        base.getEventBase().newEvent([id, this] {timeout(id);}, 0);
      query.timeout->add(base.getQueryTimeout());
      active[id] = query;
      return true;
    }
    writeWaiting(true);
    return false;

  } CATCH_DEBUG(4);

  failures++;
  return false;
}


void Nameserver::timeout(uint16_t id) {
  auto it = active.find(id);
  if (it == active.end()) return;
  auto query = it->second;
  active.erase(it);
  respond(query, new Result(DNS_ERR_TIMEOUT), 0);
  if (!waiting) base.schedule();
}


void Nameserver::writeWaiting(bool waiting) {
  if (this->waiting == waiting) return;
  this->waiting = waiting;
  LOG_DEBUG(5, "Write waiting " << (waiting ? "true" : "false"));

  event->renew(
    socket->get(), EVENT_READ | EVENT_PERSIST | (waiting ? EVENT_WRITE : 0));
  event->add();
}


void Nameserver::read() {
  uint8_t packet[1500];
  SockAddr addr;
  auto r = socket->read(packet, sizeof(packet), 0, &addr);
  if (!r) return;

  if (addr != this->addr)
    THROW("DNS response from unexpected address " << addr);
  else {
    auto id         = hton16(*(uint16_t *)(packet + 0));
    auto flags      = hton16(*(uint16_t *)(packet + 2));
    auto questions  = hton16(*(uint16_t *)(packet + 4));
    auto answers    = hton16(*(uint16_t *)(packet + 6));
    unsigned offset = 12;

    if (!(flags & 0x8000)) THROW("DNS response is not an answer");

    // Check request ID
    auto it = active.find(id);
    if (it == active.end()) THROW("DNS request with ID " << id << " not found");

    auto query    = it->second;
    auto result   = SmartPtr(new Result((Error::enum_t)(flags & 0x20f)));
    unsigned rTTL = 0;

    // Check that response matches request
    if (questions == 1) {
      string name = parseName(packet, r, offset);
      offset += 4; // Skip rest of question section <u16:type><u16:class>

      if (query.request == name) active.erase(it);
      else THROW("DNS response does not match request: " << name << " != "
                 << query.request);
    } else THROW("Expected one question in DNS response");

    // Answers
    try {
      for (unsigned i = 0; i < answers; i++) {
        parseName(packet, r, offset); // Skip name

        auto type  = hton16(*(uint16_t *)(packet + offset + 0));
        auto klass = hton16(*(uint16_t *)(packet + offset + 2));
        auto ttl   = hton32(*(uint32_t *)(packet + offset + 4));
        auto len   = hton16(*(uint32_t *)(packet + offset + 8));
        offset += 10;

        if (klass == CLASS_INET && type == query.type) {
          rTTL = ttl;

          switch (type) {
          case DNS_IPV4: {
            if (len & 3) THROW("Invalid IPV4 length in DNS response");
            unsigned count = len >> 2;

            for (unsigned j = 0; j < count; j++) {
              auto addr = hton32(*(uint32_t *)(packet + offset + 4 * j));
              result->addrs.push_back(addr);
            }
            break;
          }

          case DNS_PTR: {
            auto l = offset;
            result->names.push_back(parseName(packet, r, l));
            break;
          }


          case DNS_IPV6: {
            if (len & 15) THROW("Invalid IPV6 length in DNS response");
            unsigned count = len >> 4;

            for (unsigned j = 0; j < count; j++) {
              SockAddr addr(packet + offset + 16 * j);
              result->addrs.push_back(addr);
            }
            break;
          }
          }
        }

        offset += len;
      }
    } CATCH_DEBUG(4);

    query.timeout->del();
    respond(query, result, rTTL);
  }
}


void Nameserver::ready(Event::Event &e, int fd, unsigned flags) {
  try {
    LOG_DEBUG(5, "Ready " << ((flags & EVENT_READ) ? "READ" : "")
              << ((flags & EVENT_WRITE) ? "WRITE" : ""));

    if (flags & EVENT_READ) read();
    if (flags & EVENT_WRITE) writeWaiting(false);
    base.schedule();
  } CATCH_DEBUG(4);
}


void Nameserver::respond(
  const Query &query, const SmartPointer<Result> &result, unsigned ttl) {
  if (result->error && result->error != DNS_ERR_NOTEXIST) failures++;
  else failures = 0;
  base.response(query.type, query.request, result, ttl);
}
