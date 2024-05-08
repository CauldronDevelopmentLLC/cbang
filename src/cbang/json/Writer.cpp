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

#include "Writer.h"

#include "Integer.h"

#include <cbang/String.h>
#include <cbang/Catch.h>
#include <cbang/SStream.h>

#include <cctype>
#include <iomanip>
#include <sstream>
#include <cmath>


using namespace std;
using namespace cb::JSON;


Writer::~Writer() {TRY_CATCH_ERROR(close());}


void Writer::close() {
  NullSink::close();
  stream.flush();
}


void Writer::reset() {
  NullSink::reset();
  stream.flush();
  simple.clear();
  first = true;
}


void Writer::writeNull() {
  NullSink::writeNull();
  stream << "null";
}


void Writer::writeBoolean(bool value) {
  NullSink::writeBoolean(value);
  stream << (value ? "true" : "false");
}


void Writer::write(double value) {
  NullSink::write(value);

  // These values are parsed correctly by both Python and Javascript
  if (std::isnan(value)) stream << "\"NaN\"";
  else if (std::isinf(value) && 0 < value) stream << "\"Infinity\"";
  else if (std::isinf(value) && value < 0) stream << "\"-Infinity\"";
  else stream << cb::String(value, precision);
}


void Writer::write(uint64_t value) {
  NullSink::write(value);
  stream << cb::String(value);
}


void Writer::write(int64_t value) {
  NullSink::write(value);
  stream << cb::String(value);
}


void Writer::write(const string &value) {
  NullSink::write(value);
  stream << '"' << escape(value) << '"';
}


void Writer::beginList(bool simple) {
  NullSink::beginList(simple);
  this->simple.push_back(simple);
  stream << '[';
  first = true;
}


void Writer::beginAppend() {
  NullSink::beginAppend();

  if (first) first = false;
  else {
    stream << ',';
    if (simple.back() && !compact) stream << ' ';
  }

  if (!compact && !simple.back()) {
    stream << '\n';
    indent();
  }
}


void Writer::endList() {
  NullSink::endList();

  if (!(compact || simple.back()) && !first) {
    stream << '\n';
    indent();
  }

  stream << ']';

  first = false;
  simple.pop_back();
}


void Writer::beginDict(bool simple) {
  NullSink::beginDict(simple);
  this->simple.push_back(simple);
  stream << '{';
  first = true;
}


void Writer::beginInsert(const string &key) {
  NullSink::beginInsert(key);
  if (first) first = false;
  else {
    stream << ',';
    if (simple.back() && !compact) stream << ' ';
  }

  if (!simple.back() && !compact) {
    stream << '\n';
    indent();
  }

  write(key);
  stream << ':';
  if (!compact) stream << ' ';

  canWrite = true;
}



void Writer::endDict() {
  NullSink::endDict();

  if (!(simple.back() || compact) && !first) {
    stream << '\n';
    indent();
  }

  stream << '}';

  first = false;
  simple.pop_back();
}


namespace {
  string encode(unsigned c, const char *fmt) {
    return cb::String::printf(fmt, c);
  }
}


string Writer::escape(const string &s, const char *fmt) {
  string result;
  result.reserve(s.length());

  for (auto it = s.begin(); it != s.end(); it++) {
    unsigned char c = *it;

    switch (c) {
    case 0: result.append(encode(0, fmt)); break;
    case '\\': result.append("\\\\"); break;
    case '\"': result.append("\\\""); break;
    case '\b': result.append("\\b"); break;
    case '\f': result.append("\\f"); break;
    case '\n': result.append("\\n"); break;
    case '\r': result.append("\\r"); break;
    case '\t': result.append("\\t"); break;
    default:
      // Check UTF-8 encodings.
      //
      // UTF-8 code can be of the following formats:
      //
      //    Range in Hex   Binary representation
      //        0-7f       0xxxxxxx
      //       80-7ff      110xxxxx 10xxxxxx
      //      800-ffff     1110xxxx 10xxxxxx 10xxxxxx
      //    10000-1fffff   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
      //
      // See: http://en.wikipedia.org/wiki/UTF-8

      if (0x80 <= c) {
        // Compute code width
        int width;
        if ((c & 0xe0) == 0xc0) width = 1;
        else if ((c & 0xf0) == 0xe0) width = 2;
        else if ((c & 0xf8) == 0xf0) width = 3;
        else {
          // Invalid or non-standard UTF-8 code width, escape it
          result.append(encode(c, fmt));
          break;
        }

        // Check if UTF-8 code is valid
        bool valid = true;
        uint32_t code = c & (0x3f >> width);
        auto it2 = it;

        for (int i = 0; i < width; i++) {
          // Check for early end of string
          if (++it2 == s.end()) {valid = false; break;}

          // Check for invalid start bits
          if ((*it2 & 0xc0) != 0x80) {valid = false; break;}

          code = (code << 6) | (*it2 & 0x3f);
        }

        if (!valid) result.append(encode(*it, fmt)); // Encode character
        else {
          if (0x2000 <= code && code < 0x2100)
            // Always encode Javascript line separators
            result.append(encode(code, fmt));

          else result.append(it, it2 + 1); // Otherwise, pass valid UTF-8

          it = it2;
        }

      } else if (iscntrl(c)) // Always encode control characters
        result.append(encode(c, fmt));

      else result.push_back(c); // Pass normal characters
      break;
    }
  }

  return result;
}


void Writer::indent() const {
  stream << string((getDepth() + indentStart) * indentSpace, ' ');
}
