/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "URLHandler.h"

#include <cbang/http/URLPatternMatcher.h>

using namespace std;
using namespace cb;
using namespace cb::API;


URLHandler::URLHandler(
  const string &pattern, const SmartPointer<Handler> &child) :
  re(HTTP::URLPatternMatcher::toRE2Pattern(pattern), false), child(child) {}


bool URLHandler::operator()(const CtxPtr &ctx) {
  Regex::Match m;
  string path = ctx->getRequest().getURI().getPath();

  if (!re.match(path, m)) return false;

  // Handle URL args
  auto &names = re.getGroupIndexMap();
  if (!names.empty()) {
    auto args = ctx->getArgs()->copy();

    // Look up the indices of named groups and get their matched values
    // Note, URL args are always strings but they may be later converted to
    // other types by the ArgValidator.
    for (unsigned i = 0; i < re.getGroupCount(); i++) {
      auto it = names.find(i + 1);

      // TODO Currently, if an arg name already exists it is not overwritten.
      // The consequence is that the API caller can override URL args with
      // JSON or query args.  This is intentional but may not be ideal.
      if (it != names.end() && !args->has(it->second))
        args->insert(it->second, m[i]);
    }

    CtxPtr childCtx = new Context(*ctx);
    childCtx->setArgs(args);
    return child->operator()(childCtx);
  }

  return child->operator()(ctx);
}
