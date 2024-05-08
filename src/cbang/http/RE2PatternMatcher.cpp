/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#include <vector>

#include <re2/re2.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


struct RE2PatternMatcher::Private {
  RE2 regex;
  Private(const string &s) : regex(s) {}
};


RE2PatternMatcher::RE2PatternMatcher(
  const string &pattern, const SmartPointer<RequestHandler> &child) :
  pri(new Private(pattern)), child(child) {
  if (pri->regex.error_code())
    THROW("Failed to compile RE2: " << pri->regex.error());

  if (child.isNull()) THROW("Child cannot be NULL");

  // Regex args
  const auto &names = pri->regex.CapturingGroupNames();
  for (auto &it: names) if (!it.second.empty()) args.insert(it.second);
}


bool RE2PatternMatcher::match(const URI &uri, JSON::ValuePtr resultArgs) const {
  int n = pri->regex.NumberOfCapturingGroups();
  vector<RE2::Arg>   args(n);
  vector<RE2::Arg *> argPtrs(n);
  vector<string>     results(n);

  // Connect args
  for (int i = 0; i < n; i++) {
    args[i]    = &results[i];
    argPtrs[i] = &args[i];
  }

  // Attempt match
  string path = uri.getPath();
  if (!RE2::FullMatchN(path, pri->regex, argPtrs.data(), n)) {
    LOG_DEBUG(6, path << " did not match " << pri->regex.pattern());
    return false;
  }

  LOG_DEBUG(5, path << " matched " << pri->regex.pattern());

  // Store results
  if (resultArgs.isSet()) {
    auto &names = pri->regex.CapturingGroupNames();

    for (int i = 0; i < n; i++)
      if (!results[i].empty()) {
        auto it = names.find(i + 1);

        if (it != names.end() && !resultArgs->has(it->second))
          resultArgs->insert(it->second, results[i]);
      }
  }

  return true;
}


bool RE2PatternMatcher::operator()(Request &req) {
  if (!match(req.getURI(), req.getArgs())) return false;
  return (*child)(req);
}
