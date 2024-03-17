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

#include "Connector.h"

#include <cbang/config/Options.h>

using namespace cb;
using namespace cb::MariaDB;


void Connector::addOptions(Options &options) {
  options.pushCategory("Database");
  options.addTarget("db-host", dbHost, "DB host name");
  options.addTarget("db-user", dbUser, "DB user name");
  options.addTarget("db-pass", dbPass, "DB password")->setObscured();;
  options.addTarget("db-name", dbName, "DB name");
  options.addTarget("db-port", dbPort, "DB port");
  options.addTarget("db-timeout", dbTimeout, "DB timeout");
  options.popCategory();
}


SmartPointer<MariaDB::EventDB> Connector::getConnection() {
  // TODO Limit the total number of active connections

  auto db = SmartPtr(new MariaDB::EventDB(base));

  // Configure
  db->setConnectTimeout(dbTimeout);
  db->setReadTimeout(dbTimeout);
  db->setWriteTimeout(dbTimeout);
  db->setReconnect(true);
  db->enableNonBlocking();
  db->setCharacterSet("utf8");

  // Connect
  db->connectNB(dbHost, dbUser, dbPass, dbName, dbPort);

  return db;
}
