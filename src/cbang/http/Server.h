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

#include "HandlerGroup.h"

#include <cbang/event/Server.h>
#include <cbang/net/URI.h>
#include <cbang/util/Version.h>


namespace cb {
  class SSLContext;

  namespace HTTP {
    class Conn;

    class Server : public Event::Server, public HandlerGroup {
      SmartPointer<SSLContext> sslCtx;

      bool logPrefix = false;
      int priority = -1;
      int securePriority = -1;

      unsigned maxBodySize   = std::numeric_limits<int>::max();
      unsigned maxHeaderSize = std::numeric_limits<int>::max();

    public:
      Server(Event::Base &base, const SmartPointer<SSLContext> &sslCtx = 0);

      const SmartPointer<SSLContext> &getSSLContext() const {return sslCtx;}

      bool getLogPrefix() const {return logPrefix;}
      void setLogPrefix(bool logPrefix) {this->logPrefix = logPrefix;}

      int getPortPriority() const {return priority;}
      void setPortPriority(int priority) {this->priority = priority;}

      int getSecurePortPriority() const {return securePriority;}
      void setSecurePortPriority(int p) {this->securePriority = p;}

      unsigned getMaxBodySize() const {return maxBodySize;}
      void setMaxBodySize(unsigned size) {maxBodySize = size;}

      unsigned getMaxHeaderSize() const {return maxHeaderSize;}
      void setMaxHeaderSize(unsigned size) {maxHeaderSize = size;}

      void addListenPort(const SockAddr &addr);
      void addSecureListenPort(const SockAddr &addr);

      // From Event::Server
      void addOptions(Options &options) override;
      void init(Options &options) override;
      SmartPointer<Event::Connection> createConnection() override;

      virtual SmartPointer<Request> createRequest(const RequestParams &params);
      virtual void endRequest(Request &req);

      void dispatch(Request &req);

      // From RequestHandler
      bool operator()(Request &req) override;
    };
  }
}
