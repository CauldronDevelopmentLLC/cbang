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

#include "HTTPHandler.h"

#include <cbang/SmartPointer.h>
#include <cbang/util/Base.h>
#include <cbang/net/IPAddress.h>

#include <vector>

struct evhttp;
struct event_base;
struct bufferevent;


namespace cb {
  class SSLContext;
  class IPAddress;

  namespace Event {
    class Base;

    class HTTP {
      evhttp *http;
      SmartPointer<HTTPHandler> handler;
      SmartPointer<SSLContext> sslCtx;
      int priority;

      cb::IPAddress boundAddr;

    public:
      HTTP(const Base &base, const SmartPointer<HTTPHandler> &handler,
           const SmartPointer<SSLContext> &sslCtx = 0);
      virtual ~HTTP();

      void setMaxBodySize(unsigned size);
      void setMaxHeadersSize(unsigned size);
      void setTimeout(int timeout);
      void setEventPriority(int priority) {this->priority = priority;}
      void setMaxConnections(unsigned x);
      int getConnectionCount() const;
      void setMaxConnectionTTL(unsigned x);

      int bind(const IPAddress &addr);
      const cb::IPAddress &getBoundAddress() const {return boundAddr;}

      bufferevent *bevCB(event_base *base);
      void requestCB(evhttp_request *req);

      static bool dispatch(HTTPHandler &handler, Request &req);
    };
  }
}
