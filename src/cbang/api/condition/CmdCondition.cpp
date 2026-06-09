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

#include "CmdCondition.h"
#include "CmdConditionProc.h"

#include <cbang/api/API.h>
#include <cbang/api/Resolver.h>
#include <cbang/os/Subprocess.h>
#include <cbang/event/SubprocessPool.h>

using namespace std;
using namespace cb;
using namespace cb::API;


CmdCondition::CmdCondition(API &api, const JSON::ValuePtr &config) : api(api) {
  if (config->isString()) Subprocess::parse(config->asString(), cmd);
  else if (config->isList())
    for (auto &v: *config) cmd.push_back(v->asString());
  else THROW("'cmd' must be a string or list");
}


void CmdCondition::operator()(const CtxPtr &ctx, const BoolCallback &done) {
  auto resolver = ctx->getResolver();

  vector<string> rcmd;
  for (auto &arg: cmd) rcmd.push_back(resolver->resolve(arg));

  auto &pool = api.getProcPool();
  pool.enqueue(new CmdConditionProc(pool.getBase(), rcmd, done));
}
