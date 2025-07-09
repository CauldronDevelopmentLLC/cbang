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

#include <cbang/String.h>
#include <cbang/json/Dict.h>


namespace cb {
  namespace HTTP {
    class Request;
    class Session;
  }

  namespace API {
    class API;
    class Resolver;
    using ResolverPtr = SmartPointer<Resolver>;

    class Resolver : virtual public RefCounted {
      SmartPointer<Resolver> parent;
      JSON::Dict vars;

    public:
      Resolver(const SmartPointer<Resolver> parent = 0) : parent(parent) {}
      Resolver(API &api);
      Resolver(API &api, const HTTP::Request &req);
      virtual ~Resolver() {}

      void set(const std::string &key, const JSON::ValuePtr &values);
      void setSession(const SmartPointer<HTTP::Session> &session);

      virtual JSON::ValuePtr select(const std::string &path) const;
      std::string selectString(const std::string &path) const;
      std::string selectString(
        const std::string &path, const std::string &defaultValue) const;
      uint64_t selectU64(const std::string &path, uint64_t defaultValue) const;
      uint64_t selectTime(const std::string &path, uint64_t defaultValue) const;

      std::string resolve(const std::string &s, bool sql = false) const;
      void resolve(JSON::Value &value, bool sql = false) const;
    };

    using ResolverPtr = SmartPointer<Resolver>;
  }
}
