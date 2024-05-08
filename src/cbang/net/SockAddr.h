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

#pragma once

#include "SocketType.h"

#include <cbang/util/Regex.h>

#include <cstdint>
#include <iostream>

struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;


namespace cb {
  class SockAddr {
    uint8_t *data;

    static Regex ipv4RE;
    static Regex ipv6RE;

  public:
    SockAddr();
    SockAddr(const SockAddr &o) : SockAddr() {*this = o;}
    SockAddr(const sockaddr &addr);
    SockAddr(uint32_t ip, uint16_t port = 0);
    SockAddr(const uint8_t *ip, uint16_t port = 0);
    ~SockAddr();

    bool isNull() const;
    bool isZero() const;
    unsigned getLength() const;
    static unsigned getCapacity();

    const sockaddr *get() const {return (const sockaddr *)data;}
    sockaddr *get() {return (sockaddr *)data;}

    const sockaddr_in *get4() const {return (const sockaddr_in *)data;}
    sockaddr_in *get4() {return (sockaddr_in *)data;}

    const sockaddr_in6 *get6() const {return (const sockaddr_in6 *)data;}
    sockaddr_in6 *get6() {return (sockaddr_in6 *)data;}

    bool isLoopback() const;

    bool isIPv4() const;
    bool isIPv6() const;

    uint32_t getIPv4() const;
    const uint8_t *getIPv6() const;

    void setIPv4(uint32_t ip);
    void setIPv6(const uint8_t *ip);

    unsigned getPort() const;
    void setPort(unsigned port);

    void clear();

    std::string toString(bool withPort = true) const;

    bool readIPv4(const std::string &s);
    bool readIPv6(const std::string &s);
    bool read(const std::string &s);

    static SockAddr parseIPv4(const std::string &s);
    static SockAddr parseIPv6(const std::string &s);
    static SockAddr parse(const std::string &s);

    static bool isIPv4Address(const std::string &s);
    static bool isIPv6Address(const std::string &s);
    static bool isAddress(const std::string &s);

    SockAddr &operator=(const SockAddr &o);
    SockAddr &operator=(const sockaddr &addr);
    SockAddr &operator=(const sockaddr_in &addr);
    SockAddr &operator=(const sockaddr_in6 &addr);

    int cmp(const SockAddr &o, bool cmpPorts = true) const;
    bool operator< (const SockAddr &a) const {return cmp(a) <  0;}
    bool operator<=(const SockAddr &a) const {return cmp(a) <= 0;}
    bool operator> (const SockAddr &a) const {return cmp(a) >  0;}
    bool operator>=(const SockAddr &a) const {return cmp(a) >= 0;}
    bool operator==(const SockAddr &a) const {return cmp(a) == 0;}
    bool operator!=(const SockAddr &a) const {return cmp(a) != 0;}

    void setCIDRBits(uint8_t bits, bool on);
    int8_t getCIDRBits(const SockAddr &end) const;

    bool adjacent(const SockAddr &o) const;

    void bind(socket_t socket) const;
    socket_t accept(socket_t socket);
    void connect(socket_t socket) const;

  protected:
    SockAddr dec() const;
  };


  inline static std::ostream &operator<<(
    std::ostream &stream, const SockAddr &sa) {
    stream << sa.toString();
    return stream;
  }
}
