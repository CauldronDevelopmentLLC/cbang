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

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/os/SysError.h>
#include <cbang/net/Swab.h>

#include <string.h>

#ifdef _WIN32
#include "Winsock.h"
#include <ws2tcpip.h>
#include <ws2ipdef.h>

typedef int socklen_t;  // Unix socket length
#define SOCKET_INPROGRESS WSAEWOULDBLOCK

#else // !_WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define INVALID_SOCKET -1        // WinSock invalid socket
#define SOCKET_ERROR   -1        // Basic WinSock error
#define SOCKET_INPROGRESS EINPROGRESS

#endif // !_WIN32

using namespace std;
using namespace cb;


#define U8_RE "((2((5[0-5])|([0-4]\\d)))|(1\\d\\d)|([1-9]?\\d))"

#define U16_RE \
  "((6((5((5((3[0-5])|([0-2]\\d)))|[0-4]\\d\\d))|[0-4]\\d{3}))|" \
  "([0-5]\\d{4})|([1-9]\\d{0,3})|0)"

#define PORT_RE "(:" U16_RE ")"
#define IPV4_RE U8_RE "\\." U8_RE "\\." U8_RE "\\." U8_RE
#define IPV6_RE "([\\da-fA-F:\\.]+)" // Imprecise

Regex SockAddr::ipv4RE(IPV4_RE PORT_RE "?");
Regex SockAddr::ipv6RE("(" IPV6_RE ")|(\\[" IPV6_RE "\\]" PORT_RE "?)");


SockAddr::SockAddr() : data(new uint8_t[getCapacity()]()) {}


SockAddr::SockAddr(const sockaddr &addr) : SockAddr() {
  if (addr.sa_family == AF_INET6) memcpy(data, &addr, sizeof(sockaddr_in6));
  else if (addr.sa_family == AF_INET) memcpy(data, &addr, sizeof(sockaddr_in));
  else THROW("Unsupported sockaddr family: " << addr.sa_family);
}


SockAddr::SockAddr(uint32_t ip, uint16_t port) : SockAddr() {
  setIPv4(ip);
  setPort(port);
}


SockAddr::SockAddr(uint8_t *ip, uint16_t port) : SockAddr() {
  setIPv6(ip);
  setPort(port);
}


SockAddr::~SockAddr() {delete [] data;}


bool SockAddr::isEmpty() const {return !getLength();}


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


string SockAddr::toString() const {
  unsigned port = getPort();
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


SockAddr SockAddr::parseIPv4(const string &_s) {
  string s = _s;
  if (!ipv4RE.match(s)) THROW("Invalid address: " << s);

  uint16_t port = 0;
  if (s.find_last_of(':') != string::npos) {
    size_t end = s.find_last_of(':');
    port = String::parseU16(s.substr(end + 1));
    s    = s.substr(0, end);
  }

  SockAddr addr;
  addr.setIPv4(0);
  addr.setPort(port);
  if (inet_pton(AF_INET, s.data(), &addr.get4()->sin_addr) != 1)
    THROW("Invalid address: " << s);

  return addr;
}


SockAddr SockAddr::parseIPv6(const string &_s) {
  string s = _s;
  if (!ipv6RE.match(s)) THROW("Invalid address: " << s);

  unsigned port = 0;
  if (s[0] == '[') {
    size_t end = s.find_last_of(']');
    if (end + 1 < s.length())
      port = String::parseU16(s.substr(end + 2));
    s = s.substr(1, end - 1);
  }

  SockAddr addr;
  addr.setIPv6(0);
  addr.setPort(port);
  if (inet_pton(AF_INET6, s.data(), &addr.get6()->sin6_addr) != 1)
    THROW("Invalid address: " << s);

  return addr;
}


SockAddr SockAddr::parse(const string &s) {
  if (ipv4RE.match(s)) return parseIPv4(s);
  return parseIPv6(s);
}


SockAddr &SockAddr::operator=(const SockAddr &o) {
  memcpy(data, o.data, getCapacity());
  return *this;
}


bool SockAddr::operator<(const SockAddr &o) const {
  if (get()->sa_family != o.get()->sa_family)
    return get()->sa_family < o.get()->sa_family;

  if (isEmpty()) return false;

  if (isIPv4() && getIPv4() != o.getIPv4()) return getIPv4() < o.getIPv4();
  if (isIPv6()) {
    auto a = getIPv6();
    auto b = o.getIPv6();
    int cmp = memcmp(a, b, 16);
    if (cmp) return cmp < 0;
  }

  if (getPort() != o.getPort()) return getPort() < o.getPort();

  return false;
}


int SockAddr::compare(const SockAddr &o) const {
  if (*this < o) return -1;
  if (o < *this) return  1;
  return 0;
}


void SockAddr::bind(socket_t socket) const {
  if (isEmpty()) THROW("Cannot bind to empty SockAddr");

  SysError::clear();
  if (::bind(socket, get(), getLength()))
    THROW("Could not bind socket to " << *this << ": " << SysError());
}


socket_t SockAddr::accept(socket_t socket) {
  socklen_t len = getCapacity();
  return ::accept(socket, get(), &len);
}


void SockAddr::connect(socket_t socket) const {
  if (isEmpty()) THROW("Cannot connect to empty SockAddr");

  SysError::clear();
  if (::connect(socket, get(), getLength()) == -1)
    if (SysError::get() != SOCKET_INPROGRESS)
      THROW("Failed to connect to " << *this << ": " << SysError());
}
