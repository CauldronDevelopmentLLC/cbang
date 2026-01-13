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

#include <cbang/json/Value.h>
#include <cbang/json/Builder.h>
#include <cbang/http/Status.h>
#include <cbang/db/maria/EventDB.h>


namespace cb {
  namespace API {
    class API;
    class QueryDef;

    class Query : public RefCounted, public HTTP::Status {
    public:
      using state_t    = MariaDB::EventDB::state_t;
      using return_t   = void (Query::*)(state_t);
      using callback_t =
        std::function<void (HTTP::Status, const JSON::ValuePtr &)>;

    protected:
      const QueryDef &def;
      callback_t cb;

      SmartPointer<MariaDB::EventDB> db;

      std::string nextField;
      unsigned currentField = 0;
      bool closeField       = true;
      unsigned rowCount     = 0;

      JSON::Builder sink;

    public:
      Query(const QueryDef &def, callback_t cb);
      virtual ~Query() {}

      void exec(const std::string &sql);
      static return_t getReturnType(const std::string &name);

    protected:
      virtual void callback(state_t state);

      void reply(HTTP::Status code = HTTP_OK);
      void errorReply(HTTP::Status code, const std::string &msg = "");

      // MariaDB::EventDB callbacks
      void returnHList (state_t state);
      void returnList  (state_t state);
      void returnBool  (state_t state);
      void returnU64   (state_t state);
      void returnS64   (state_t state);
      void returnFields(state_t state);
      void returnDict  (state_t state);
      void returnOne   (state_t state);
      bool checkOne    (state_t state);
      void returnOk    (state_t state);
    };
  }
}
