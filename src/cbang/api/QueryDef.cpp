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

#include "QueryDef.h"
#include "API.h"
#include "Resolver.h"

#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::API;


QueryDef::QueryDef(API &api, const JSON::ValuePtr &config) :
  api(api), sql(String::trim(config->getString("sql", ""))) {

  if (sql.empty()) THROW("Query must have 'sql'");

  if (config->hasList("fields")) fields = config->get("fields");
  ret = config->getString("return", fields.isNull() ? "ok" : "fields");
  Query::getReturnType(ret); // Check that return type is valid
}


SmartPointer<MariaDB::EventDB> QueryDef::getDBConnection() const {
  return api.getDBConnector().getConnection();
}


SmartPointer<Query> QueryDef::query(
  const string &sql, Query::callback_t cb) const {

  auto query = SmartPtr(new Query(*this, cb));
  query->exec(sql);
  return query;
}


SmartPointer<Query> QueryDef::query(
  const ResolverPtr &resolver, Query::callback_t cb) const {
  return query(resolver->resolve(sql, true), cb);
}
