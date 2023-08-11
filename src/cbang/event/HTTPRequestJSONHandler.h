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

#pragma once

#include "HTTPRequestHandler.h"

#include <cbang/json/Value.h>

namespace cb {
  namespace Event {
    struct HTTPRequestJSONHandler : public HTTPRequestHandler {
      virtual void operator()(Request &, const JSON::ValuePtr &) = 0;

      // From HTTPRequestHandler
      bool operator()(Request &req) override;
    };


    template <class T>
    struct HTTPRequestJSONMemberHandler : public HTTPRequestJSONHandler {
      typedef void (T::*member_t)(Request &, const JSON::ValuePtr &);
      T *obj;
      member_t member;

      HTTPRequestJSONMemberHandler(T *obj, member_t member) :
        obj(obj), member(member) {
        if (!obj) CBANG_THROW("Object cannot be NULL");
        if (!member) CBANG_THROW("Member cannot be NULL");
      }

      // From HTTPRequestJSONHandler
      void operator()(Request &req, const JSON::ValuePtr &msg) override
        {(*obj.*member)(req, msg);}
    };


    template <class T>
    struct HTTPRequestJSONRecastHandler : public HTTPRequestJSONHandler {
      typedef void (T::*member_t)(Request &, const JSON::ValuePtr &);
      member_t member;

      HTTPRequestJSONRecastHandler(member_t member) : member(member) {
        if (!member) CBANG_THROW("Member cannot be NULL");
      }

      // From HTTPRequestJSONHandler
      void operator()(Request &req, const JSON::ValuePtr &msg) override {
        (req.cast<T>().*member)(req, msg);
      }
    };
  }
}
