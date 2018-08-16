/******************************************************************************\

         This file is part of the C! library.  A.K.A the cbang library.

               Copyright (c) 2003-2018, Cauldron Development LLC
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

#include "HTTPURLPatternMatcher.h"

#include <cbang/String.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb::Event;


HTTPURLPatternMatcher::HTTPURLPatternMatcher
(const string &pattern, const SmartPointer<HTTPHandler> &child) {
  string rePattern;

  if (!pattern.empty() && pattern[0] == '^') rePattern = pattern;
  else {
    vector<string> parts;
    String::tokenize(pattern, parts, "/");

    for (unsigned i = 0; i < parts.size(); i++)
      if (!parts[i].empty() && parts[i][0] == ':') {
        string arg = parts[i].substr(1);
        if (!args.insert(arg).second)
          THROWS("Duplicate argument '" << arg << "' in URL pattern: "
                 << pattern);
        rePattern += "/(?P<" + arg + ">[^/]*)";

      } else rePattern += "/" + parts[i];
  }

  LOG_DEBUG(5, "HTTPURLPatternMatcher() " << pattern << " -> " << rePattern);

  matcher = new HTTPRE2PatternMatcher(rePattern, "", child);
}
