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
  api(api), sql(String::trim(config->getString("sql", ""))),
  contentType(config->getString("content-type", "")) {

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
  query->setContentType(contentType);
  query->exec(sql);
  return query;
}


SmartPointer<Query> QueryDef::query(
  const ResolverPtr &resolver, Query::callback_t cb) const {
  vector<string> params;
  string s = resolver->resolveSQL(sql, params);

  // Check the bound parameters match the ? placeholders, counting quote-aware
  // so a binary ref resolved inside a string literal is caught here with a
  // clear error rather than failing at execute.
  unsigned placeholders = 0;
  bool quoted = false;
  for (char c: s) {
    if (c == '\'') quoted = !quoted;
    else if (c == '?' && !quoted) placeholders++;
  }
  if (placeholders != params.size())
    THROW("SQL has " << placeholders << " placeholders but "
          << params.size() << " bound parameters; a binary {ref} cannot be "
          "embedded in a string");

  auto query = SmartPtr(new Query(*this, cb));
  query->setContentType(
    contentType.empty() ? contentType : resolver->resolve(contentType));
  query->exec(s, params);
  return query;
}
