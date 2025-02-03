/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "Error.h"
#include "Type.h"

#include <cbang/net/Socket.h>
#include <cbang/event/EventFlag.h>

#include <map>


namespace cb {
  namespace Event {class Event;}

  namespace DNS {
    class Base;
    class Request;
    class Result;

    class Nameserver :
      public Event::EventFlag, public Error::Enum, public Type::Enum {
      Base &base;
      SockAddr addr;
      bool system;

      SmartPointer<Socket> socket;
      SmartPointer<Event::Event> event;

      struct Query {
        Type type;
        std::string request;
        SmartPointer<Event::Event> timeout;
      };

      std::map<uint16_t, Query> active;

      unsigned failures = 0;
      bool waiting = false;

    public:
      Nameserver(Base &base, const SockAddr &addr, bool system);
      ~Nameserver();

      const SockAddr &getAddress() const {return addr;}
      bool isSystem() const {return system;}
      unsigned getFailures() const {return failures;}

      void start();
      void stop();

      bool transmit(Type type, const std::string &request);

    protected:
      void timeout(uint16_t id);
      void writeWaiting(bool waiting);
      void read();
      void ready(Event::Event &e, int fd, unsigned flags);
      void respond(const Query &query, const cb::SmartPointer<Result> &result,
                   unsigned ttl);
    };
  }
}
