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

#include "Condition.h"
#include "ExistsCondition.h"
#include "CompareCondition.h"
#include "NotCondition.h"
#include "BoolOpCondition.h"
#include "SQLCondition.h"
#include "CmdCondition.h"
#include "TruthyCondition.h"

#include <cbang/Exception.h>
#include <cbang/json/Value.h>

#include <vector>

using namespace std;
using namespace cb;
using namespace cb::API;


bool cb::API::Condition::truthy(const JSON::ValuePtr &v) {
  if (v.isNull() || v->isNull() || v->isUndefined()) return false;
  if (v->isBoolean()) return v->getBoolean();
  if (v->isNumber())  return v->getNumber() != 0;
  if (v->isString())  return !v->getString().empty();
  if (v->isList() || v->isDict()) return v->size();
  return true;
}


// Fully qualified so `Condition` is unambiguous despite cb::Condition.
ConditionPtr cb::API::Condition::parse(
  API &api, const JSON::ValuePtr &config) {
  // A bare scalar value (a ref or literal) is a truthiness test
  if (!config->isDict() && !config->isList())
    return new TruthyCondition(config);

  if (!config->isDict() || config->size() != 1)
    THROW("Condition must be a dict with exactly one key");

  string op;
  JSON::ValuePtr val;
  for (auto e: config->entries()) {op = e.key(); val = e.value();}

  if (op == "exists") return new ExistsCondition(val);
  if (op == "=" || op == "!=" || op == "<" || op == "<=")
    return new CompareCondition(op, val);

  if (op == "not") return new NotCondition(parse(api, val));

  if (op == "and" || op == "or") {
    if (!val->isList()) THROW("'" << op << "' takes a list of conditions");
    vector<ConditionPtr> conds;
    for (auto &c: *val) conds.push_back(parse(api, c));
    return new BoolOpCondition(op == "and", conds);
  }

  if (op == "sql") return new SQLCondition(api, val);
  if (op == "cmd") return new CmdCondition(api, val);

  THROW("Unknown condition '" << op << "'");
}
