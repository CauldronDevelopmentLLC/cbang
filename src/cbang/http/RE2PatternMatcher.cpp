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

#include "RE2PatternMatcher.h"
#include "Request.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


RE2PatternMatcher::RE2PatternMatcher(
  const string &pattern, const SmartPointer<RequestHandler> &child) :
  re(pattern, false), child(child) {
  if (child.isNull()) THROW("Child cannot be null");
}


bool RE2PatternMatcher::match(const URI &uri, JSON::ValuePtr resultArgs) const {
  Regex::Match m;
  string path = uri.getPath();

  if (!re.match(path, m)) {
    LOG_DEBUG(6, path << " did not match " << re);
    return false;
  }

  LOG_DEBUG(5, path << " matched " << re);

  // Store results
  if (resultArgs.isSet()) {
    auto &names = re.getGroupIndexMap();

    for (unsigned i = 0; i < re.getGroupCount(); i++) {
      auto it = names.find(i + 1);

      if (it != names.end() && !resultArgs->has(it->second))
        resultArgs->insert(it->second, m[i]);
    }
  }

  return true;
}


bool RE2PatternMatcher::operator()(Request &req) {
  if (!match(req.getURI(), req.getArgs())) return false;
  return (*child)(req);
}
