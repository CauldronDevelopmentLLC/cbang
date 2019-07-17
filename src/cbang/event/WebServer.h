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
#include "HTTPHandlerGroup.h"

#include <cbang/net/IPAddressFilter.h>


namespace cb {
  class SSLContext;
  class Options;

  namespace Event {
    class Base;
    class HTTP;
    class Request;

    class WebServer : public HTTPHandlerGroup, public HTTPHandler {
      Options &options;
      SmartPointer<SSLContext> sslCtx;

      SmartPointer<HTTP> http;
      SmartPointer<HTTP> https;

      bool initialized;

      IPAddressFilter ipFilter;

      typedef std::vector<IPAddress> ports_t;
      ports_t ports;
      ports_t securePorts;

      bool logPrefix;

    public:
      WebServer(Options &options, Base &base,
                const SmartPointer<SSLContext> &sslCtx = 0,
                const SmartPointer<HTTPHandlerFactory> &factory =
                new HTTPHandlerFactory);
      virtual ~WebServer();

      void addOptions(Options &options);

      bool getLogPrefix() const {return logPrefix;}
      void setLogPrefix(bool logPrefix) {this->logPrefix = logPrefix;}

      virtual void init();
      virtual bool allow(Request &req) const;
      virtual void shutdown();

      virtual SmartPointer<Request> createRequest
      (RequestMethod method, const URI &uri, const Version &version);

      // From HTTPHandler
      SmartPointer<Request> createRequest
      (Connection &con, RequestMethod method, const URI &uri,
       const Version &version);
      bool handleRequest(Request &req);
      void endRequest(Request &req);

      const SmartPointer<SSLContext> &getSSLContext() const {return sslCtx;}

      const SmartPointer<HTTP> &getHTTP() const {return http;}
      const SmartPointer<HTTP> &getHTTPS() const {return https;}

      void addListenPort(const IPAddress &addr);
      unsigned getNumListenPorts() const {return ports.size();}
      const IPAddress &getListenPort(unsigned i) const {return ports.at(i);}

      void setEventPriority(int priority);
      void setMaxConnections(unsigned x);
      void setMaxConnectionTTL(unsigned x);

      void allow(const IPAddress &addr);
      void deny(const IPAddress &addr);

      void addSecureListenPort(const IPAddress &addr);
      unsigned getNumSecureListenPorts() const {return securePorts.size();}
      const IPAddress &getSecureListenPort(unsigned i) const
      {return securePorts.at(i);}

      void setMaxBodySize(unsigned size);
      void setMaxHeadersSize(unsigned size);
      void setTimeout(int timeout);
    };
  }
}
