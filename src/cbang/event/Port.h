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

#include "Enum.h"

#include <cbang/SmartPointer.h>
#include <cbang/net/SockAddr.h>
#include <cbang/openssl/SSLContext.h>


namespace cb {
  class Socket;

  namespace Event {
    class Server;
    class Event;

    class Port : public Enum {
      Server &server;
      SockAddr addr;
      SmartPointer<SSLContext> sslCtx;
      int priority;

      SmartPointer<Socket> socket;
      SmartPointer<Event> event;

    public:
      Port(Server &server, const SockAddr addr,
           const SmartPointer<SSLContext> &sslCtx, int priority);
      ~Port();

      const SockAddr &getAddr() const {return addr;}
      bool isSecure() const {return sslCtx.isSet();}

      int getPriority() const {return priority;}
      void setPriority(int priority);

      void open();
      void close();

      void activate();
      void accept();
    };
  }
}
