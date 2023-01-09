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

#include <cbang/time/TimeInterval.h>
#include <cbang/String.h>
#include <cbang/time/Time.h>

using namespace std;
using namespace cb;


string TimeInterval::toString() const {
  const unsigned divs[] = {
    Time::SEC_PER_YEAR, Time::SEC_PER_DAY, Time::SEC_PER_HOUR,
    Time::SEC_PER_MIN, 1};
  const char *longNames[]  = {"y", "d", "m", "h", "s"};
  const char *shortNames[] = {" year", " day", " min", " hour", " sec"};
  const char **names       = compact ? longNames : shortNames;
  unsigned i = interval < 0 ? -interval : interval;

  for (unsigned j = 0; j < 4; j++)
    if (divs[j] < i) {
      int a = (int)interval / divs[j];
      int b = ((int)interval % divs[j]) / divs[j + 1];

      return String::printf("%d%s%s %d%s%s",
                            a, names[j + 0], (compact || a == 1) ? "" : "s",
                            b, names[j + 1], (compact || b == 1) ? "" : "s");
    }

  return String::printf("%d%s%s", (int)interval, names[4],
                        (compact || (int)interval == 1) ? "" : "s");
}
