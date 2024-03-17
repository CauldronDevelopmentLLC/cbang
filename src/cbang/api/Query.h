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

#include <cbang/json/Value.h>
#include <cbang/http/Request.h>
#include <cbang/event/JSONBufferWriter.h>
#include <cbang/db/maria/EventDB.h>


namespace cb {
  namespace API {
    class API;

    class Query : public HTTP::Status {
    protected:
      API &api;
      SmartPointer<HTTP::Request> req;
      std::string sql;
      std::string returnType;
      JSON::ValuePtr fields;

      std::string nextField;
      unsigned currentField = 0;
      bool closeField       = true;
      unsigned rowCount     = 0;

      typedef std::function<void (HTTP::Status, Event::Buffer &)> callback_t;
      callback_t cb;

      SmartPointer<MariaDB::EventDB> db;
      Event::JSONBufferWriter writer;

    public:
      Query(API &api, const SmartPointer<HTTP::Request> &req,
            const std::string &sql, const std::string &returnType = "ok",
            const JSON::ValuePtr &fields = 0);
      virtual ~Query() {}

      void query(callback_t cb);

      typedef MariaDB::EventDB::state_t state_t;
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
      void returnOk    (state_t state);
    };
  }
}
