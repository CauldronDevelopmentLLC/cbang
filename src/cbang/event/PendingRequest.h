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

#include "Request.h"
#include "Connection.h"

#include <cbang/SmartPointer.h>

#include <functional>


namespace cb {
  class URI;

  namespace Event {
    class Client;
    class HTTPHandler;

    class PendingRequest : public Connection, public Request {
    public:
      typedef std::function<void (Request *, int)> callback_t;

    protected:
      Client &client;
      unsigned method;
      callback_t cb;
      int err;
      SmartPointer<Request> selfRef;

    public:
      PendingRequest(Client &client, const URI &uri, unsigned method,
                     callback_t cb);
      ~PendingRequest();

      Client &getClient() {return client;}
      RequestMethod getMethod() const {return (RequestMethod::enum_t)method;}

      using Request::send;
      void send();

      void callback();
      void error(int code);
    };

    typedef SmartPointer<PendingRequest> PendingRequestPtr;
  }
}
