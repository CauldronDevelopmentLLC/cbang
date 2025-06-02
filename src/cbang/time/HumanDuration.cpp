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

#include "HumanDuration.h"
#include "Time.h"

#include <cbang/String.h>

#include <iomanip>
#include <cctype>
#include <set>

using namespace std;
using namespace cb;


void HumanDuration::print(ostream &stream) const {
  uint64_t t = duration;
  uint64_t sec = t % 60; t /= 60;
  uint64_t min = t % 60; t /= 60;
  uint64_t hour = t % 24; t /= 24;
  uint64_t day = t % 365; t /= 365;

  if (t)    stream <<               t << "y ";
  if (day)  stream << setw(3) << day  << "d ";
  if (hour) stream << setw(2) << hour << "h ";
  if (min)  stream << setw(2) << min  << "m ";
  stream           << setw(2) << sec  << "s";
}


uint64_t HumanDuration::parse(const string &s) {
  uint64_t t = 0;
  set<char> found;

  unsigned i = 0;
  while (i < s.length()) {
    // Skip whitespace
    while (i < s.length() && isspace(s[i])) i++;
    if (i == s.length()) break;

    // Parse number
    if (!isdigit(s[i]))
      THROW("Expected digit in duration '" << s << "' at character " << i);

    uint64_t x = 0;
    while (i < s.length() && isdigit(s[i]))
      x = 10 * x + (s[i++] - '0');

    // Skip whitespace
    while (i < s.length() && isspace(s[i])) i++;

    // Parse unit, assume seconds if at end of string
    char unit = i == s.length() ? 's' : s[i++];

    if (!found.insert(unit).second)
      THROW("Time unit '" << unit << "' already specifed in duration '" << s
        << "' at character " << (i - 1));

    switch (unit) {
      case 'y': t += x * Time::SEC_PER_YEAR; break;
      case 'd': t += x * Time::SEC_PER_DAY;  break;
      case 'h': t += x * Time::SEC_PER_HOUR; break;
      case 'm': t += x * Time::SEC_PER_MIN;  break;
      case 's': t += x; break;
      default:
        THROW("Expected time unit in duration '" << s
          << "' at character " << (i - 1));
    }
  }

  if (found.empty()) THROW("Expected duration");

  return t;
}
