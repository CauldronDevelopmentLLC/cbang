/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
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

#include <cbang/api/Resolver.h>
#include <cbang/api/Websocket.h>
#include <cbang/http/Request.h>

#include <functional>

namespace cb {
  namespace API {
    class API;
    class Websocket;

    class Context : public cb::HTTP::Status::Enum {
      API &api;
      HTTP::Request &req;
      WebsocketPtr ws;
      JSON::ValuePtr args;
      SmartPointer<Resolver> resolver;

    public:
      Context(API &api, HTTP::Request &req);
      Context(API &api, const WebsocketPtr &ws);

      HTTP::Request &getRequest() const {return req;}
      const WebsocketPtr &getWebsocket() const {return ws;}

      const SmartPointer<Resolver> &getResolver() const {return resolver;}
      void setResolver(const SmartPointer<Resolver> &resolver)
        {this->resolver = resolver;}

      const JSON::ValuePtr &getArgs() const {return args;}
      void setArgs(const JSON::ValuePtr &args) {this->args = args;}

      void setSession(const SmartPointer<HTTP::Session> &session);

      void reply(HTTP::Status code, const JSON::ValuePtr &msg) const;
      void reply(const JSON::ValuePtr &msg) const;
      void reply(HTTP::Status code,
        std::function<void (JSON::Sink &sink)> cb) const;
      void reply(std::function<void (JSON::Sink &sink)> cb) const;
      void reply(HTTP::Status code = HTTP_OK,
        const std::string &text = "") const;
      void reply(const std::exception &e) const;
      void reply(const Exception &e) const;

      void errorHandler(std::function<void ()> cb) const;
    };

    using CtxPtr = SmartPointer<Context>;
  }
}
