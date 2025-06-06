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

#include "Time.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cbang/boost/StartInclude.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cbang/boost/EndInclude.h>

#include <sstream>
#include <locale>
#include <exception>

using namespace std;
using namespace cb;

namespace pt = boost::posix_time;

namespace {
  const boost::gregorian::date epoch(1970, 1, 1);
}


Time::Time(uint64_t time) : time(time == ~(uint64_t)0 ? now() : time) {}


Time::Time(const struct tm &tm) :
  time((pt::ptime_from_tm(tm) - pt::ptime(epoch)).total_seconds()) {}


string Time::toString(const string &format) const {
  if (!time) return "<invalid>";

  try {
    pt::time_facet *facet = new pt::time_facet();
    facet->format(format.c_str());

    pt::ptime t(epoch, pt::seconds(time));
    stringstream ss;
    ss.imbue(locale(ss.getloc(), facet));
    ss << t;

    return ss.str();

  } catch (const exception &e) {
    THROW("Failed to format time '" << time << "' with format '" << format
           << "': " << e.what());
  }
}


uint64_t Time::parse(const string &s, const string &format) {
  try {
    pt::time_input_facet *facet = new pt::time_input_facet();
    facet->format(format.c_str());

    pt::ptime t;
    istringstream ss(s);
    ss.imbue(locale(ss.getloc(), facet));
    ss >> t;

    if (ss.fail()) THROW("Parse failed");
    ss.get();
    if (!ss.eof()) THROW("Did not parse whole string");

    pt::time_duration diff = t - pt::ptime(epoch);

    return diff.total_seconds();

  } catch (const exception &e) {
    THROW("Failed to parse time '" << s << "' with format '" << format
           << "': " << e.what());
  }
}


uint64_t Time::now() {
  return (uint64_t)(pt::second_clock::universal_time() -
                    pt::ptime(epoch)).total_seconds();
}


int32_t Time::offset() {
  return (int32_t)(pt::second_clock::local_time() -
                   pt::second_clock::universal_time()).total_seconds();
}
