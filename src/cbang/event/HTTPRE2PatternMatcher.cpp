/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#include "HTTPRE2PatternMatcher.h"

#include <cbang/Exception.h>
#include <cbang/event/Request.h>
#include <cbang/event/RestoreURIPath.h>
#include <cbang/log/Logger.h>

#include <vector>

using namespace std;
using namespace cb;
using namespace cb::Event;


HTTPRE2PatternMatcher::HTTPRE2PatternMatcher
(const string &search, const string &replace,
 const SmartPointer<HTTPRequestHandler> &child) :
  regex(search), replace(replace), child(child) {
  if (regex.error_code()) THROWS("Failed to compile RE2: " << regex.error());

  // Regex args
  const map<int, string> &names = regex.CapturingGroupNames();
  map<int, string>::const_iterator it;
  for (it = names.begin(); it != names.end(); it++)
    if (!it->second.empty()) args.insert(it->second);
}


bool HTTPRE2PatternMatcher::operator()(Request &req) {
  int n = regex.NumberOfCapturingGroups();
  vector<RE2::Arg> args(n);
  vector<RE2::Arg *> argPtrs(n);
  vector<string> results(n);

  // Connect args
  for (int i = 0; i < n; i++) {
    args[i] = &results[i];
    argPtrs[i] = &args[i];
  }

  // Attempt match
  URI &uri = req.getURI();
  string path = uri.getEscapedPath();
  if (!RE2::FullMatchN(path, regex, argPtrs.data(), n))
    return false;

  LOG_DEBUG(5, path << " matched " << regex.pattern());

  if (child.isNull()) return true;

  // Store results
  const map<int, string> &names = regex.CapturingGroupNames();
  for (int i = 0; i < n; i++) {
    if (results[i].empty()) continue;

    if (names.find(i + 1) != names.end())
      req.insertArg(names.at(i + 1), results[i]);
    else req.insertArg(results[i]);
  }

  // Replace path
  RestoreURIPath restoreURIPath(uri);
  if (!replace.empty() && RE2::Replace(&path, regex, replace))
    uri.setPath(path);

  // Call child
  return (*child)(req);
}
