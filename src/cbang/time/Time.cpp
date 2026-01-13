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

#include "Time.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cbang/boost/StartInclude.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cbang/boost/EndInclude.h>

#include <sstream>
#include <locale>
#include <exception>
#include <ctime>

#ifdef _WIN32
#define timegm _mkgmtime
#endif

using namespace std;
using namespace cb;

namespace pt = boost::posix_time;

namespace {
  const boost::gregorian::date epoch(1970, 1, 1);


  bool consume(
    string::const_iterator &it, const string::const_iterator &end, char c) {
    if (it == end || *it != c) return false;
    it++;
    return true;
  }


  bool isDigit(
    const string::const_iterator &it, const string::const_iterator &end) {
    return it != end && '0' <= *it && *it <= '9';
  }


  uint32_t parseUInt(string::const_iterator &it,
    const string::const_iterator &end, unsigned digits) {
    uint32_t x = 0;

    for (unsigned i = 0; i < digits; i++) {
      if (isDigit(it, end)) x = x * 10 + *it++ - '0';
      else THROW("Expected digit");
    }

    return x;
  }


  uint64_t parseISO8601(const string &s) {
    auto it  = s.begin();
    auto end = s.end();

    try {
      auto year = parseUInt(it, end, 4);
      if (year < 1970) THROW("Unsupported year " << year);
      consume(it, end, '-');

      auto month = parseUInt(it, end, 2);
      if (!month || 12 < month) THROW("Invalid month " << month);
      consume(it, end, '-');

      auto day = parseUInt(it, end, 2);
      if (!day || 31 < day) THROW("Invalid day " << day);

      if (!(consume(it, end, ' ') || consume(it, end, 'T')))
        THROW("Expected 'T' or ' '");

      auto hour = parseUInt(it, end, 2);
      if (23 < hour) THROW("Invalid hour " << hour);
      consume(it, end, ':');

      auto min = parseUInt(it, end, 2);
      if (59 < min) THROW("Invalid minute " << min);
      consume(it, end, ':');

      auto sec = parseUInt(it, end, 2);
      if (59 < sec) THROW("Invalid second " << sec);

      if (consume(it, end, '.'))
        for (unsigned i = 0; i < 9; i++)
          if (isDigit(it, end)) it++;

      consume(it, end, 'Z');

      if (it != end)
        THROW("Did not parse whole string, '" << string(it, end)
          << "' remains");

      struct tm t{};
      t.tm_year = year  - 1900;
      t.tm_mon  = month - 1;
      t.tm_mday = day;
      t.tm_hour = hour;
      t.tm_min  = min;
      t.tm_sec  = sec;

      auto result = timegm(&t);
      if (result == -1) THROW("Invalid time");
      return result;

    } catch (const Exception &e) {
      THROWC("Failed to parse ISO8601 time '" << s << "' at character "
        << (it - s.begin()), e);
    }
  }
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
    if (ss.tellg() != (streampos)s.length()) {
      string remains;
      ss >> remains;
      THROW("Did not parse whole string, '" << remains << "' remains");
    }

    pt::time_duration diff = t - pt::ptime(epoch);

    return diff.total_seconds();

  } catch (const exception &e) {
    THROW("Failed to parse time '" << s << "' with format '" << format
           << "': " << e.what());
  }
}


uint64_t Time::parse(const string &s) {return parseISO8601(s);}


uint64_t Time::now() {
  return (uint64_t)(pt::second_clock::universal_time() -
                    pt::ptime(epoch)).total_seconds();
}


int32_t Time::offset() {
  return (int32_t)(pt::second_clock::local_time() -
                   pt::second_clock::universal_time()).total_seconds();
}
