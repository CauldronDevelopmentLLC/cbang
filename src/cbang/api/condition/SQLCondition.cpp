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

#include "SQLCondition.h"

#include <cbang/api/API.h>
#include <cbang/api/Resolver.h>
#include <cbang/json/Dict.h>
#include <cbang/json/String.h>

using namespace std;
using namespace cb;
using namespace cb::API;


namespace {
  // Truthiness: set and not false/empty/zero/null (see doc/conditions.md).
  bool truthy(const JSON::ValuePtr &v) {
    if (v.isNull() || v->isNull() || v->isUndefined()) return false;
    if (v->isBoolean()) return v->getBoolean();
    if (v->isNumber())  return v->getNumber() != 0;
    if (v->isString())  return !v->getString().empty();
    if (v->isList() || v->isDict()) return v->size();
    return true;
  }
}


SQLCondition::SQLCondition(API &api, const JSON::ValuePtr &config) {
  auto qc = SmartPtr(new JSON::Dict);
  qc->insert("sql", new JSON::String(config->asString()));
  qc->insert("return", new JSON::String("one"));
  queryDef = new QueryDef(api, qc);
}


void SQLCondition::operator()(const CtxPtr &ctx, const BoolCallback &done) {
  queryDef->query(ctx->getResolver(),
    [done] (HTTP::Status status, const JSON::ValuePtr &result) {
      unsigned code = status;
      done(200 <= code && code < 300 && truthy(result));
    });
}
