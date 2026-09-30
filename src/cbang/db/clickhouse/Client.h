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

#include "Response.h"

#include <cbang/SmartPointer.h>

#include <functional>


namespace cb {
  class Options;
  class URI;
  namespace HTTP {class Client;}

  namespace ClickHouse {
    // An asynchronous client for the ClickHouse HTTP interface
    class Client {
      SmartPointer<HTTP::Client> client;

      std::string url      = "http://127.0.0.1:8123/";
      std::string user     = "default";
      std::string password;
      std::string readerUser;
      std::string readerPassword;
      std::string database;
      unsigned    timeout  = 30;

    public:
      using callback_t = std::function<void (const Response &)>;

      Client(const SmartPointer<HTTP::Client> &client) : client(client) {}

      void addOptions(Options &options);

      const std::string &getURL()            const {return url;}
      const std::string &getUser()           const {return user;}
      const std::string &getPassword()       const {return password;}
      const std::string &getReaderUser()     const {return readerUser;}
      const std::string &getReaderPassword() const {return readerPassword;}
      const std::string &getDatabase()       const {return database;}
      unsigned           getTimeout()        const {return timeout;}

      void setURL     (const std::string &url)      {this->url      = url;}
      void setUser    (const std::string &user)     {this->user     = user;}
      void setPassword(const std::string &password) {this->password = password;}
      void setDatabase(const std::string &database) {this->database = database;}
      void setTimeout (unsigned timeout)            {this->timeout  = timeout;}

      void setReaderUser(const std::string &user) {readerUser = user;}
      void setReaderPassword(const std::string &password)
        {readerPassword = password;}

      // Run ``sql`` with ``params``, a dict of values for the server-side
      // ``{name:Type}`` parameters, and a dict of extra ClickHouse
      // ``settings``.  Either dict may be null.  A ``readOnly`` query uses
      // the reader login, if one is set.
      void query(const std::string &sql, const JSON::ValuePtr &params,
                 const JSON::ValuePtr &settings, callback_t cb,
                 bool readOnly = false);

      // Insert a list of row objects as JSONEachRow.  ``table`` is used as
      // given, so it must be a trusted ``[db.]table`` name.
      void insert(const std::string &table, const JSON::Value &rows,
                  const JSON::ValuePtr &settings, callback_t cb);

      // A query parameter value in ClickHouse's text format
      static std::string formatParam(const JSON::Value &value);

    protected:
      URI getURI(const JSON::ValuePtr &settings) const;
      void send(const URI &uri, const std::string &data, callback_t cb,
                bool readOnly);
    };
  }
}
