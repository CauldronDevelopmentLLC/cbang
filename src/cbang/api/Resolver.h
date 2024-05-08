/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include <cbang/String.h>
#include <cbang/json/Dict.h>
#include <cbang/http/Request.h>

#include <functional>


namespace cb {
  namespace API {
    class API;
    class Resolver;
    typedef SmartPointer<Resolver> ResolverPtr;
    typedef SmartPointer<HTTP::Request> RequestPtr;


    class Resolver : virtual public RefCounted {
      API &api;
      RequestPtr req;
      JSON::ValuePtr ctx;
      ResolverPtr parent;

      Resolver(API &api, const JSON::ValuePtr &ctx, const ResolverPtr &parent);

    public:
      Resolver(API &api, const RequestPtr &req);
      virtual ~Resolver() {}

      API &getAPI() {return api;}
      const JSON::ValuePtr &getContext() const {return ctx;}

      ResolverPtr makeChild(const JSON::ValuePtr &ctx);

      virtual JSON::ValuePtr select(const std::string &name) const;
      std::string format(const std::string &s,
                         String::format_cb_t cb = 0) const;
      std::string format(const std::string &s,
                         const std::string &defaultValue) const;
      void resolve(JSON::Value &value) const;
    };
  }
}
