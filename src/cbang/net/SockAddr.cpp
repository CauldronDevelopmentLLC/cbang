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

#include "SockAddr.h"
#include "Swab.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/os/SysError.h>

#include <string.h>

#ifdef _WIN32
#include "Winsock.h"
#include <ws2tcpip.h>
#include <ws2ipdef.h>

typedef int socklen_t;
#define SOCKET_INPROGRESS WSAEWOULDBLOCK

#else // !_WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define SOCKET_INPROGRESS EINPROGRESS

#endif // !_WIN32

using namespace std;
using namespace cb;


namespace {
  template <typename T> int compare(const T &a, const T &b) {
    return a < b ? -1 : (b < a ? 1 : 0);
  }
}


#define U8_RE "((2((5[0-5])|([0-4]\\d)))|(1\\d\\d)|([1-9]?\\d))"

#define U16_RE \
  "((6((5((5((3[0-5])|([0-2]\\d)))|[0-4]\\d\\d))|[0-4]\\d{3}))|" \
  "([0-5]\\d{4})|([1-9]\\d{0,3})|0)"

#define PORT_RE "(:" U16_RE ")"
#define IPV4_RE U8_RE "\\." U8_RE "\\." U8_RE "\\." U8_RE
#define IPV6_RE "([\\da-fA-F:\\.]+)(%.+)?" // Imprecise

Regex SockAddr::ipv4RE("((" IPV4_RE ")|(\\d+))" PORT_RE "?");
Regex SockAddr::ipv6RE("(" IPV6_RE ")|(\\[" IPV6_RE "\\]" PORT_RE "?)");


SockAddr::SockAddr() : data(new uint8_t[getCapacity()]()) {}
SockAddr::SockAddr(const sockaddr &addr) : SockAddr() {*this = addr;}


SockAddr::SockAddr(uint32_t ip, uint16_t port) : SockAddr() {
  setIPv4(ip);
  setPort(port);
}


SockAddr::SockAddr(const uint8_t *ip, uint16_t port) : SockAddr() {
  setIPv6(ip);
  setPort(port);
}


SockAddr::~SockAddr() {delete [] data;}


bool SockAddr::isNull() const {return !getLength();}


bool SockAddr::isZero() const {
  if (isIPv4()) return !getIPv4();

  if (isIPv6()) {
    auto addr = getIPv6();

    for (int i = 0; i < 16; i++)
      if (addr[i]) return false;

    return true;
  }

  return isNull();
}


unsigned SockAddr::getLength() const {
  if (isIPv4()) return sizeof(sockaddr_in);
  if (isIPv6()) return sizeof(sockaddr_in6);
  return 0;
}


unsigned SockAddr::getCapacity() {return sizeof(sockaddr_storage);}


bool SockAddr::isLoopback() const {
  if (isIPv4()) return (getIPv4() & 0xff000000) == 0x7f000000;
  if (isIPv6())
    return !memcmp(getIPv6(), "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1", 16);
  return false;
}


bool SockAddr::isIPv4() const {return get()->sa_family == AF_INET;}
bool SockAddr::isIPv6() const {return get()->sa_family == AF_INET6;}


unsigned SockAddr::getIPv4() const {
  if (!isIPv4()) THROW("Not an IPv4 address");
  return hton32(get4()->sin_addr.s_addr);
}


const uint8_t *SockAddr::getIPv6() const {
  if (!isIPv6()) THROW("Not an IPv6 address");
  return get6()->sin6_addr.s6_addr;
}


void SockAddr::setIPv4(uint32_t ip) {
  auto port = getPort();
  clear();

  auto addr        = get4();
  addr->sin_family = AF_INET;
  addr->sin_port   = hton16(port);
  addr->sin_addr.s_addr = hton32(ip);
}


void SockAddr::setIPv6(const uint8_t *ip) {
  auto port = getPort();
  clear();

  auto addr         = get6();
  addr->sin6_family = AF_INET6;
  addr->sin6_port   = hton16(port);

  if (ip) memcpy(addr->sin6_addr.s6_addr, ip, 16);
}


unsigned SockAddr::getPort() const {
  if (isIPv4()) return hton16(get4()->sin_port);
  if (isIPv6()) return hton16(get6()->sin6_port);
  return 0;
}


void SockAddr::setPort(unsigned port) {
  if (isIPv4()) get4()->sin_port  = hton16(port);
  if (isIPv6()) get6()->sin6_port = hton16(port);
}


void SockAddr::clear() {memset(data, 0, getCapacity());}


string SockAddr::toString(bool withPort) const {
  unsigned port = withPort ? getPort() : 0;
  string s;

  if (isIPv4()) {
    unsigned ip = getIPv4();
    s = String::printf(
      "%u.%u.%u.%u", ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);

    if (port) s += ":" + String(port);

  } else if (isIPv6()) {
    char buf[64];

    if (!inet_ntop(AF_INET6, &get6()->sin6_addr, buf, 64))
      THROW("Failed to convert IPv6 address to string: " << SysError());

    s = buf;

    if (port) s = "[" + s + "]:" + String(port);

  } else s = "<unknown sockaddr>";

  return s;
}


bool SockAddr::readIPv4(const string &_s) {
  if (!ipv4RE.match(_s)) return false;

  string s = _s;
  uint16_t port = 0;
  if (s.find_last_of(':') != string::npos) {
    size_t end = s.find_last_of(':');
    port = String::parseU16(s.substr(end + 1));
    s    = s.substr(0, end);
  }

  // Special case
  if (s.find_first_not_of("1234567890") == string::npos) {
    setIPv4(String::parseU32(s));
    setPort(port);
    return true;
  }

  if (inet_pton(AF_INET, s.data(), &get4()->sin_addr) == 1) {
    get()->sa_family = AF_INET;
    setPort(port);
    return true;
  }

  return false;
}


bool SockAddr::readIPv6(const string &_s) {
  if (!ipv6RE.match(_s)) return false;

  // Parse port
  string s = _s;
  unsigned port = 0;
  if (s[0] == '[') {
    size_t end = s.find_last_of(']');
    if (end + 1 < s.length()) port = String::parseU16(s.substr(end + 2));
    s = s.substr(1, end - 1);
  }

  // Strip zone ID, inet_pton() does not handle zone ID
  // TODO use getaddrinfo() with AI_NUMERICHOST which handles zone IDs
  size_t percent = s.find('%');
  if (percent != string::npos) s = s.substr(0, percent);

  // Parse address
  if (inet_pton(AF_INET6, s.data(), &get6()->sin6_addr) == 1) {
    get()->sa_family = AF_INET6;
    setPort(port);
    return true;
  }

  return false;
}


bool SockAddr::read(const string &s) {return readIPv4(s) || readIPv6(s);}


SockAddr SockAddr::parseIPv4(const string &s) {
  SockAddr addr;
  if (!addr.readIPv4(s)) THROW("Invalid IPv4 address: " << s);
  return addr;
}


SockAddr SockAddr::parseIPv6(const string &s) {
  SockAddr addr;
  if (!addr.readIPv6(s)) THROW("Invalid IPv6 address: " << s);
  return addr;
}


SockAddr SockAddr::parse(const string &s) {
  SockAddr addr;
  if (!addr.read(s)) THROW("Invalid socket address: " << s);
  return addr;
}


bool SockAddr::isIPv4Address(const string &s) {return SockAddr().readIPv4(s);}
bool SockAddr::isIPv6Address(const string &s) {return SockAddr().readIPv6(s);}
bool SockAddr::isAddress(const string &s)     {return SockAddr().read(s);}


SockAddr &SockAddr::operator=(const SockAddr &o) {
  memcpy(data, o.data, o.getCapacity());
  return *this;
}


SockAddr &SockAddr::operator=(const sockaddr &addr) {
  switch (addr.sa_family) {
  case AF_INET:  return *this = (sockaddr_in  &)addr;
  case AF_INET6: return *this = (sockaddr_in6 &)addr;
  default: THROW("Unsupported sockaddr family: " << addr.sa_family);
  }
}


SockAddr &SockAddr::operator=(const sockaddr_in &addr) {
  memcpy(data, &addr, sizeof(addr));
  return *this;
}


SockAddr &SockAddr::operator=(const sockaddr_in6 &addr) {
  memcpy(data, &addr, sizeof(addr));
  return *this;
}


int SockAddr::cmp(const SockAddr &o, bool cmpPorts) const {
  if (isNull())   return o.isNull() ? 0 : -1;
  if (o.isNull()) return 1;

  if (get()->sa_family != o.get()->sa_family)
    return compare(get()->sa_family, o.get()->sa_family);

  if (isIPv4() && getIPv4() != o.getIPv4())
    return compare(getIPv4(), o.getIPv4());

  if (isIPv6()) {
    auto a  = getIPv6();
    auto b  = o.getIPv6();
    int ret = memcmp(a, b, 16);
    if (ret) return ret;
  }

  if (cmpPorts) return compare(getPort(), o.getPort());
  return 0;
}


void SockAddr::setCIDRBits(uint8_t bits, bool on) {
  if (isIPv4()) {
    bits = 32 - bits;
    auto addr     = getIPv4();
    uint32_t mask = (1ULL << bits) - 1;

    if (on) addr |= mask;
    else addr &= ~mask;

    get4()->sin_addr.s_addr = hton32(addr);

  } else if (isIPv6()) {
    bits = 128 - bits;
    auto addr = get6()->sin6_addr.s6_addr;

    for (int i = 15; 0 <= i && bits; i--) {
      if (8 <= bits) {
        addr[i] = on ? 0xff : 0;
        bits -= 8;

      } else {
        uint8_t mask = (1 << bits) - 1;
        if (on) addr[i] |= mask;
        else addr[i] &= ~mask;
        break;
      }
    }

  } else THROW("Cannot set range bits on " << *this);
}


int8_t SockAddr::getCIDRBits(const SockAddr &o) const {
  if (isIPv4() && o.isIPv4()) {
    uint32_t s = getIPv4();
    uint32_t e = o.getIPv4();

    bool inMask = false;
    uint8_t prefix = 0;

    for (int i = 31; 0 <= i; i--) {
      uint32_t mask = 1 << i;

      if (!inMask) {
        if ((s & mask) == (e & mask)) prefix++;
        else inMask = true;
      }

      if (inMask && ((s & mask) || !(e & mask))) return -1;
    }

    return prefix;
  }

  if (isIPv6() && o.isIPv6()) {
    auto *s = getIPv6();
    auto *e = o.getIPv6();

    bool inMask = false;
    uint8_t prefix = 0;

    for (int i = 0; i < 16; i++)
      for (int j = 7; 0 <= j; j--) {
        uint8_t mask = 1 << j;

        if (!inMask) {
          if ((s[i] & mask) == (e[i] & mask)) prefix++;
          else inMask = true;
        }

        if (inMask && ((s[i] & mask) || !(e[i] & mask))) return -1;
      }

    return prefix;
  }

  THROW("Cannot get range bits from " << *this << " and " << o);
}


bool SockAddr::adjacent(const SockAddr &o) const {
  return (!o.isZero() && cmp(o.dec(), false) == 0) ||
    (!isZero() && dec().cmp(o, false) == 0);
}


void SockAddr::bind(socket_t socket) const {
  if (isNull()) THROW("Cannot bind to null SockAddr");

  SysError::clear();
  if (::bind(socket, get(), getLength()))
    THROW("Could not bind socket to " << *this << ": " << SysError());
}


socket_t SockAddr::accept(socket_t socket) {
  socklen_t len = getCapacity();
  return ::accept(socket, get(), &len);
}


void SockAddr::connect(socket_t socket) const {
  if (isNull()) THROW("Cannot connect to null SockAddr");

  SysError::clear();
  if (::connect(socket, get(), getLength()) == -1)
    if (SysError::get() != SOCKET_INPROGRESS)
      THROW("Failed to connect to " << *this << ": " << SysError());
}


SockAddr SockAddr::dec() const {
  if (isIPv4()) return SockAddr(getIPv4() - 1, getPort());

  if (isIPv6()) {
    uint8_t addr[16];
    memcpy(addr, getIPv6(), 16);

    for (int i = 15; 0 <= i; i--) {
      if (addr[i]) {addr[i]--; break;}
      else addr[i] = 0xff;
    }

    return SockAddr(addr, getPort());
  }

  THROW("Cannot decrement address " << *this);
}
