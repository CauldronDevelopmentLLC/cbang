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

#pragma once

#include "Connection.h"
#include "Port.h"

#include <cbang/SmartPointer.h>
#include <cbang/openssl/SSLContext.h>
#include <cbang/net/AddressFilter.h>

#include <list>
#include <set>
#include <limits>


namespace cb {
  class Socket;
  class Options;


  namespace Event {
    class Server : public Enum {
      Base &base;

      typedef std::list<SmartPointer<Port>> ports_t;
      ports_t ports;

      typedef std::set<SmartPointer<Connection>> connections_t;
      connections_t connections;

      int readTimeout = 50;
      int writeTimeout = 50;
      unsigned maxConnections = std::numeric_limits<unsigned>::max();
      unsigned maxConnectionTTL = 0;
      unsigned connectionBacklog = 128;

      AddressFilter addrFilter;

      SmartPointer<RateSet> stats;

    public:
      Server(Base &base);
      virtual ~Server() {}

      Base &getBase() {return base;}

      const ports_t &getPorts() const {return ports;}
      const connections_t &getConnections() const {return connections;}

      int getReadTimeout() const {return readTimeout;}
      void setReadTimeout(unsigned t) {readTimeout = t;}

      int getWriteTimeout() const {return writeTimeout;}
      void setWriteTimeout(unsigned t) {writeTimeout = t;}

      void setTimeout(int timeout);

      unsigned getMaxConnections() const {return maxConnections;}
      void setMaxConnections(unsigned x) {maxConnections = x;}

      unsigned getMaxConnectionTTL() const {return maxConnectionTTL;}
      void setMaxConnectionTTL(unsigned x) {maxConnectionTTL = x;}

      unsigned getConnectionBacklog() const {return connectionBacklog;}
      void setConnectionBacklog(unsigned x) {connectionBacklog = x;}

      void allow(const std::string &spec);
      void deny(const std::string &spec);

      const SmartPointer<RateSet> &getStats() const {return stats;}
      void setStats(const SmartPointer<RateSet> &stats) {this->stats = stats;}

      unsigned getConnectionCount() const {return connections.size();}

      virtual void addOptions(Options &options);
      virtual void init(Options &options);

      void bind(const SockAddr &addr,
                const SmartPointer<SSLContext> &sslCtx = 0, int priority = -1);
      void shutdown();

      void accept(const SockAddr &peerAddr, const SmartPointer<Socket> &socket,
                  const SmartPointer<SSLContext> &sslCtx);
      void remove(const SmartPointer<Connection> &conn);

      virtual bool isAllowed(const SockAddr &peerAddr) const;
      virtual SmartPointer<Connection> createConnection();
    };
  }
}
