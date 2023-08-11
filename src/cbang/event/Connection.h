/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include "FD.h"
#include "Enum.h"

#include <cbang/net/IPAddress.h>
#include <cbang/time/Time.h>
#include <cbang/util/RateSet.h>

#include <functional>


namespace cb {
  class Socket;
  class SSLContext;

  namespace Event {
    class DNSBase;
    class Server;

    class Connection : public FD, public Enum {
      SmartPointer<Event> timeout;

      SmartPointer<Socket> socket;
      IPAddress peer;

      static uint64_t nextID;
      uint64_t id = ++nextID;

      SmartPointer<RateSet> stats;

    public:
      Connection(Base &base);
      ~Connection();

      void setTTL(double sec);

      const SmartPointer<Socket> &getSocket() {return socket;}
      void setSocket(const SmartPointer<Socket> &socket);

      void setPeer(const IPAddress &peer) {this->peer = peer;}
      const IPAddress &getPeer() const {return peer;}

      uint64_t getID() const {return id;}

      const SmartPointer<RateSet> &getStats() const {return stats;}
      void setStats(const SmartPointer<RateSet> &stats) {this->stats = stats;}

      bool isConnected() const;
      void accept(const IPAddress &peer, const SmartPointer<Socket> &socket,
                  const SmartPointer<SSLContext> &sslCtx);
      void connect(DNSBase &dns, const IPAddress &peer, const IPAddress &bind,
                   const SmartPointer<SSLContext> &sslCtx,
                   std::function<void (bool)> cb);

    protected:
      void timedout();
    };
  }
}
