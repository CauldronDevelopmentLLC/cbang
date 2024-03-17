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

#include "Server.h"
#include "HandlerGroup.h"

#include <cbang/net/AddressFilter.h>


namespace cb {
  class SSLContext;
  class Options;
  namespace Event {class Base;}

  namespace HTTP {
    class HTTP;
    class Request;

    class WebServer : public HandlerGroup, public Server {
      Options &options;
      SmartPointer<SSLContext> sslCtx;

      bool logPrefix = false;
      int priority = -1;

      AddressFilter addrFilter;

    public:
      WebServer(Options &options, Event::Base &base,
                const SmartPointer<SSLContext> &sslCtx = 0);
      virtual ~WebServer();

      void addOptions(Options &options);

      const SmartPointer<SSLContext> &getSSLContext() const {return sslCtx;}

      bool getLogPrefix() const {return logPrefix;}
      void setLogPrefix(bool logPrefix) {this->logPrefix = logPrefix;}

      int getEventPriority() const {return priority;}
      void setEventPriority(int priority) {this->priority = priority;}

      virtual void init();
      virtual bool allow(Request &req) const;

      void allow(const std::string &spec);
      void deny(const std::string &spec);

      void addListenPort(const SockAddr &addr);
      void addSecureListenPort(const SockAddr &addr);

      void setTimeout(int timeout);

      // From Server
      SmartPointer<Request> createRequest(
        const SmartPointer<Conn> &connection, Method method,
        const URI &uri, const Version &version) override;
      void endRequest(Request &req) override;

      // From RequestHandler
      bool operator()(Request &req) override;
    };
  }
}
