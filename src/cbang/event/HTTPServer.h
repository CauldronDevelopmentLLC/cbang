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

#include "Server.h"

#include <cbang/net/URI.h>
#include <cbang/util/Version.h>


namespace cb {
  namespace Event {
    class Request;
    class HTTPConn;

    class HTTPServer : public Server {
      unsigned maxBodySize = std::numeric_limits<unsigned>::max();
      unsigned maxHeaderSize = std::numeric_limits<unsigned>::max();

    public:
      HTTPServer(Base &base);

      unsigned getMaxBodySize() const {return maxBodySize;}
      void setMaxBodySize(unsigned size) {maxBodySize = size;}

      unsigned getMaxHeadersSize() const {return maxHeaderSize;}
      void setMaxHeadersSize(unsigned size) {maxHeaderSize = size;}

      virtual SmartPointer<Request>
      createRequest(RequestMethod method, const URI &uri,
                    const Version &version);
      virtual bool handleRequest(const SmartPointer<Request> &req) = 0;
      virtual void endRequest(const SmartPointer<Request> &req) {}

      void dispatch(const SmartPointer<Request> &req);

      // From Server
      SmartPointer<Connection> createConnection();
      void onConnect(const SmartPointer<Connection> &conn);
    };
  }
}
