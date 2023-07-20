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

#pragma once

#include <string>
#include <vector>
#include <ostream>
#include <cstdint>


namespace cb {
  /// Used for printing and comparing IP addresses
  class IPAddress {
    std::string host;
    uint32_t ip = 0; // Host byte order
    uint16_t port = 0;

  public:
    IPAddress(uint32_t ip, const std::string &host, uint16_t port = 0);
    IPAddress(uint32_t ip = 0, uint16_t port = 0);
    IPAddress(const std::string &host, uint16_t port = 0);

    void setHost(const std::string &host) {this->host = host;}
    const std::string &getHost() const {return host;}
    bool hasHost() const;
    void lookupHost();

    void setIP(uint32_t ip) {this->ip = ip;}
    uint32_t getIP() const {return ip;}

    void setPort(uint16_t port) {this->port = port;}
    uint16_t getPort() const {return port;}

    std::string toString() const;
    std::string getIPString() const {return ipToString(ip);}
    operator std::string () const {return toString();}
    operator uint32_t () const {return getIP();}

    bool operator<(const IPAddress &a) const {
      return
        getIP() == a.getIP() ? getPort() < a.getPort() : getIP() < a.getIP();
    }
    bool operator<=(const IPAddress &a) const {return !(a < *this);}
    bool operator>(const IPAddress &a) const {return a < *this;}
    bool operator>=(const IPAddress &a) const {return !(*this < a);}
    bool operator==(const IPAddress &a) const
    {return getIP() == a.getIP() && getPort() == a.getPort();}
    bool operator!=(const IPAddress &a) const {return !(*this == a);}

    static std::string ipToString(uint32_t ip);
    static uint32_t ipFromString(const std::string &host);
    static unsigned ipsFromString(const std::string &host,
                                  std::vector<IPAddress> &addrs,
                                  uint16_t port = 0, unsigned max = 0);
    static std::string hostFromIP(const IPAddress &ip);
    static uint16_t portFromString(const std::string &host);
  };

  inline std::ostream &operator<<(std::ostream &stream, const IPAddress &a) {
    return stream << a.toString();
  }
}
