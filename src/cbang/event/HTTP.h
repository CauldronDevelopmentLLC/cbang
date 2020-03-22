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
#include "Enum.h"

#include <cbang/SmartPointer.h>
#include <cbang/net/IPAddress.h>

#include <list>
#include <limits>


namespace cb {
  class SSLContext;
  class IPAddress;
  class Socket;
  class RateSet;

  namespace Event {
    class Base;
    class Event;
    class Connection;

    class HTTP : public RefCounted, public Enum {
      Base &base;

      SmartPointer<HTTPHandler> handler;
      SmartPointer<SSLContext> sslCtx;
      cb::SmartPointer<Event> expireEvent;
      cb::SmartPointer<Event> acceptEvent;

      std::string defaultContentType = "text/html; charset=UTF-8";
      unsigned maxBodySize = std::numeric_limits<unsigned>::max();
      unsigned maxHeaderSize = std::numeric_limits<unsigned>::max();
      unsigned maxConnections = std::numeric_limits<unsigned>::max();
      unsigned maxConnectionTTL = 0;
      unsigned connectionBacklog = 128;
      int readTimeout = 50;
      int writeTimeout = 50;
      int priority = -1;

      IPAddress boundAddr;
      SmartPointer<Socket> socket;
      std::list<SmartPointer<Connection> > connections;
      SmartPointer<RateSet> stats;

    public:
      HTTP(Base &base, const SmartPointer<HTTPHandler> &handler,
           const SmartPointer<SSLContext> &sslCtx = 0);
      ~HTTP();

      const std::string &getDefaultContentType() const
        {return defaultContentType;}
      void setDefaultContentType(const std::string &s) {defaultContentType = s;}

      unsigned getMaxBodySize() const {return maxBodySize;}
      void setMaxBodySize(unsigned size) {maxBodySize = size;}

      unsigned getMaxHeadersSize() const {return maxHeaderSize;}
      void setMaxHeadersSize(unsigned size) {maxHeaderSize = size;}

      int getReadTimeout() const {return readTimeout;}
      void setReadTimeout(unsigned t) {readTimeout = t;}

      int getWriteTimeout() const {return writeTimeout;}
      void setWriteTimeout(unsigned t) {writeTimeout = t;}

      unsigned getMaxConnections() const {return maxConnections;}
      void setMaxConnections(unsigned x) {maxConnections = x;}

      unsigned getMaxConnectionTTL() const {return maxConnectionTTL;}
      void setMaxConnectionTTL(unsigned x);

      unsigned getConnectionBacklog() const {return connectionBacklog;}
      void setConnectionBacklog(unsigned x) {connectionBacklog = x;}

      int getEventPriority() const {return priority;}
      void setEventPriority(int priority) {this->priority = priority;}

      unsigned getConnectionCount() const {return connections.size();}
      void remove(Connection &con);

      void setStats(const SmartPointer<RateSet> &stats) {this->stats = stats;}
      const SmartPointer<RateSet> &getStats() const {return stats;}

      void bind(const IPAddress &addr);

      SmartPointer<Request> createRequest
      (Connection &con, RequestMethod method, const URI &uri,
       const Version &version);
      void handleRequest(Request &req);

      static bool dispatch(HTTPHandler &handler, Request &req);

    protected:
      void expireCB();
      void acceptCB();
    };
  }
}
