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

#include "RequestHandler.h"

#include <cbang/event/Server.h>
#include <cbang/net/URI.h>
#include <cbang/util/Version.h>


namespace cb {
  namespace HTTP {
    class Request;
    class Conn;

    class Server : public Event::Server, private RequestHandler {
      unsigned maxBodySize   = std::numeric_limits<int>::max();
      unsigned maxHeaderSize = std::numeric_limits<int>::max();

    public:
      Server(Event::Base &base);

      unsigned getMaxBodySize() const {return maxBodySize;}
      void setMaxBodySize(unsigned size) {maxBodySize = size;}

      unsigned getMaxHeaderSize() const {return maxHeaderSize;}
      void setMaxHeaderSize(unsigned size) {maxHeaderSize = size;}

      virtual SmartPointer<Request>
      createRequest(const SmartPointer<Conn> &conn, Method method,
                    const URI &uri, const Version &version);
      virtual bool handleRequest(Request &req) = 0;
      virtual void endRequest(Request &req) {}

      void dispatch(Request &req);

      // From Event::Server
      SmartPointer<Event::Connection> createConnection() override;
      void onConnect(const SmartPointer<Event::Connection> &conn) override;

      // From RequestHandler
      bool operator()(Request &req) override {return handleRequest(req);}
    };
  }
}
