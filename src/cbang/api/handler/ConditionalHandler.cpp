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

#include "ConditionalHandler.h"

#include <cbang/api/API.h>

using namespace std;
using namespace cb;
using namespace cb::API;


ConditionalHandler::ConditionalHandler(API &api, const CfgPtr &cfg) {
  auto config = cfg->getConfig();

  if (!config->has("then")) THROW("'if' requires 'then'");

  // 'if' never sits beside other statement keys (see doc/conditions.md)
  for (auto e: config->entries()) {
    const string &key = e.key();
    if (key != "if" && key != "then" && key != "else" && key != "args" &&
        key != "allow" && key != "deny" && key != "help" && key != "hide")
      THROW("'if' cannot coexist with '" << key << "'");
  }

  // Fully qualified so `Condition` is unambiguous despite cb::Condition.
  cond        = cb::API::Condition::parse(api, config->get("if"));
  thenHandler = api.createStatementHandler(cfg->createChild(config->get("then")));

  if (config->has("else"))
    elseHandler =
      api.createStatementHandler(cfg->createChild(config->get("else")));
}


void ConditionalHandler::operator()(const CtxPtr &ctx, const Cont &next) {
  ctx->errorHandler([&] {
    (*cond)(ctx, [this, ctx, next] (bool result) {
      ctx->errorHandler([&] {
        if      (result)             (*thenHandler)(ctx, next);
        else if (elseHandler.isSet()) (*elseHandler)(ctx, next);
        else                          next(ctx);
      });
    });
  });
}
