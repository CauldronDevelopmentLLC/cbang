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

#include "EventDB.h"

#include <cbang/SmartPointer.h>


namespace cb {
  class Options;
  namespace Event {class Base;}

  namespace MariaDB {
    class Connector {
      Event::Base &base;

      std::string user;
      std::string password;
      std::string database;
      std::string host     = "127.0.0.1";
      uint32_t    port     = 3306;
      unsigned    timeout  = 5;
      int         priority = 0;

    public:
      Connector(Event::Base &base) : base(base) {}

      void addOptions(Options &options);

      const std::string &getUser()     const {return user;}
      const std::string &getPassword() const {return password;}
      const std::string &getDatabase() const {return database;}
      const std::string &getHost()     const {return host;}
      uint32_t           getPort()     const {return port;}
      unsigned           getTimeout()  const {return timeout;}
      int                getPriority() const {return priority;}

      void setUser    (const std::string &user)     {this->user     = user;}
      void setPassword(const std::string &password) {this->password = password;}
      void getDatabase(const std::string &database) {this->database = database;}
      void getHost    (const std::string &host)     {this->host     = host;}
      void getPort    (uint32_t port)               {this->port     = port;}
      void getTimeout (unsigned timeout)            {this->timeout  = timeout;}
      void getPriority(int priority)                {this->priority = priority;}

      SmartPointer<MariaDB::EventDB> getConnection();
    };
  }
}
