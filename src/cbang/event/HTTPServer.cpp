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

#include "HTTPServer.h"
#include "HTTPRequestErrorHandler.h"
#include "HTTPConnIn.h"
#include "Request.h"

#include <cbang/Catch.h>
#include <cbang/log/Logger.h>

using namespace cb::Event;
using namespace cb;
using namespace std;


HTTPServer::HTTPServer(cb::Event::Base &base) : Server(base) {}


SmartPointer<Request>
HTTPServer::createRequest(RequestMethod method, const URI &uri,
                          const Version &version) {
  return new Request(method, uri, version);
}


void HTTPServer::dispatch(Request &req) {
  HTTPRequestErrorHandler(*this)(req);
  TRY_CATCH_ERROR(endRequest(req));
}


SmartPointer<Connection> HTTPServer::createConnection() {
  return new HTTPConnIn(*this);
}


void HTTPServer::onConnect(const SmartPointer<Connection> &conn) {
  auto c = conn.cast<HTTPConnIn>();
  c->setMaxHeaderSize(maxHeaderSize);
  c->setMaxBodySize(maxBodySize);
  c->readHeader();
}
