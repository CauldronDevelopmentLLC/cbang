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

#include <cbang/http/Status.h>
#include <cbang/json/Value.h>

#include <string>


namespace cb {
  namespace HTTP {class Request;}

  namespace ClickHouse {
    // The outcome of one request.  The status is zero when there was no HTTP
    // response, because ClickHouse could not be reached or timed out.
    class Response : public HTTP::Status::Enum {
      HTTP::Status   status;
      bool           timedout = false;
      unsigned       code     = 0; // X-ClickHouse-Exception-Code
      std::string    error;
      JSON::ValuePtr json;         // The parsed JSONCompact result, if any

    public:
      static const unsigned TIMEOUT_EXCEEDED = 159;

      Response(const std::string &error, bool timedout = false) :
        timedout(timedout), error(error) {}
      Response(const HTTP::Request &req);

      bool isOk()        const {return status == HTTP_OK && error.empty();}
      bool hasResponse() const {return status;}
      bool isTimeout()   const {return timedout || code == TIMEOUT_EXCEEDED;}
      bool isDataError() const;

      HTTP::Status getStatus() const {return status;}
      unsigned getCode() const {return code;}
      const std::string &getError() const {return error;}
      const JSON::ValuePtr &getJSON() const {return json;}

      // The result's ``meta`` and ``data`` lists, empty without a result
      JSON::ValuePtr getMeta() const;
      JSON::ValuePtr getData() const;

      // The error as ``ClickHouse:<code>: <message>``
      std::string getMessage() const;
    };
  }
}
