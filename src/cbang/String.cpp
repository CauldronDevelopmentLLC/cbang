/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
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

#include "String.h"

#include "Math.h"
#include "SStream.h"
#include "Errors.h"

#include <cbang/util/Regex.h>

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstdarg>
#include <cctype>
#include <algorithm>
#include <limits>
#include <locale>
#include <cstdarg>

using namespace std;
using namespace cb;

#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#endif
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#define strtoll(p, e, b) _strtoi64(p, e, b)
#define strtoull(p, e, b) _strtoui64(p, e, b)
#define strtof(p, e) (float)strtod(p, e)
#define vsnprintf _vsnprintf
#ifndef va_copy
#define va_copy(x, y) (x = y)
#endif
#endif

const string String::DEFAULT_DELIMS = " \t\n\r";
const string String::DEFAULT_LINE_DELIMS = "\n";
const string String::LETTERS_LOWER_CASE = "abcdefghijklmnopqrstuvwxyz";
const string String::LETTERS_UPPER_CASE = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const string String::LETTERS =
  String::LETTERS_LOWER_CASE + String::LETTERS_UPPER_CASE;
const string String::NUMBERS = "0123456789";
const string String::ALPHA_NUMERIC = String::LETTERS + String::NUMBERS;


namespace {
  string toString(const char *s, string::size_type n) {
    string::size_type len = 0;
    while (len < n && s[len]) len++;

    return len ? string(s, len) : string();
  }


  string toString(double x, int precision = 6) {
    bool big = x < -1e20 || 1e20 < x;
    string s = String::printf(big ? "%.*e" : "%.*f", precision, x);

    int chop = 0;
    char point = use_facet<numpunct<char> >(cout.getloc()).decimal_point();

    for (auto it = s.rbegin(); it != s.rend(); it++)
      if (*it == '0' || *it == point) {
        chop++;
        if (*it == point) break;

      } else break;

    if (chop) s = s.substr(0, s.length() - chop);

    return s == "-0" ? "0" : s;
  }
}


String::String(const char *s, size_type n) : string(toString(s, n)) {}
String::String(double x, int precision)    : string(toString(x, precision)) {}

String::String(int8_t    x) : string(printf("%i", (int)x))                  {}
String::String(uint8_t   x) : string(printf("%u", (unsigned)x))             {}
String::String(int16_t   x) : string(printf("%i", (int)x))                  {}
String::String(uint16_t  x) : string(printf("%u", (unsigned)x))             {}
String::String(int32_t   x) : string(printf("%i", x))                       {}
String::String(uint32_t  x) : string(printf("%u", x))                       {}
String::String(int64_t   x) : string(printf("%lli", (long long int)x))      {}
String::String(uint64_t  x) : string(printf("%llu", (long long unsigned)x)) {}
String::String(double    x) : string(toString(x))                           {}
String::String(float     x) : string(toString((double)x))                   {}
String::String(bool      x) : string(x ? "true" : "false")                  {}


string String::printf(const char *format, ...) {
  va_list ap;

  va_start(ap, format);
  string result = vprintf(format, ap);
  va_end(ap);

  return result;
}


string String::vprintf(const char *format, va_list ap) {
  va_list copy;
  va_copy(copy, ap);

  int length = vsnprintf(0, 0, format, copy);
  va_end(copy);

  if (length < 0) THROW("String format '" << format << "' invalid");

  SmartPointer<char>::Array result = new char[length + 1];

  int ret = vsnprintf(result.get(), length + 1, format, ap);

  if (ret != length) THROW("String format '" << format << "' failed");

  return result.get();
}


unsigned String::tokenize(const string &s, vector<string> &tokens,
                          const string &delims, bool allowEmpty,
                          unsigned maxTokens) {
  size_t i = 0;
  unsigned count = 0;

  while (i < s.length()) {
    if (count == maxTokens - 1) {
      tokens.push_back(s.substr(i));
      count++;
      break;
    }

    if (delims.find(s[i]) != string::npos) {
      if (allowEmpty) {
        tokens.push_back(string());
        count++;
      }

      i++;
      continue;
    }

    size_t end = s.find_first_of(delims, i);
    if (end == string::npos) end = s.length();

    tokens.push_back(s.substr(i, end - i));
    count++;

    i = end + 1;
  }

  return count;
}


unsigned String::tokenizeLine(istream &stream, vector<string> &tokens,
                              const string &delims, const string &lineDelims,
                              unsigned maxLength) {
  string s;

  while (true) {
    char c = stream.get();

    if (!stream.good() || lineDelims.find(c) != string::npos) break;

    s.append(1, c);

    if (maxLength && s.length() == maxLength) break;
  }

  return tokenize(s, tokens, delims);
}


namespace cb {
  template <>
  bool String::parse<int64_t>(const string &s, int64_t &value, bool full) {
    errno = 0;
    char *end = 0;
    long long v = strtoll(s.c_str(), &end, 0);
    if (errno || v < -numeric_limits<int64_t>::max() ||
        numeric_limits<int64_t>::max() < v || (full && end && *end))
      return false;

    value = (int64_t)v;
    return true;
  }


  template <>
  bool String::parse<uint64_t>(const string &s, uint64_t &value, bool full) {
    errno = 0;
    char *end = 0;
    unsigned long long v = strtoull(s.c_str(), &end, 0);
    if (errno || numeric_limits<uint64_t>::max() < v || (full && end && *end))
      return false;
    value = (uint64_t)v;
    return true;
  }


  template <>
  bool String::parse<int32_t>(const string &s, int32_t &value, bool full) {
    errno = 0;
    char *end = 0;
    long v = strtol(s.c_str(), &end, 0);
    if (errno || v < -numeric_limits<int32_t>::max() ||
        numeric_limits<int32_t>::max() < v || (full && end && *end))
      return false;

    value = (int32_t)v;
    return true;
  }


  template <>
  bool String::parse<uint32_t>(const string &s, uint32_t &value, bool full) {
    errno = 0;
    char *end = 0;
    unsigned long v = strtoul(s.c_str(), &end, 0);
    if (errno || numeric_limits<uint32_t>::max() < v || (full && end && *end))
      return false;
    value = (uint32_t)v;
    return true;
  }


  template <>
  bool String::parse<int16_t>(const string &s, int16_t &value, bool full) {
    int32_t v;
    if (!parse<int32_t>(s, v, full) || v < -32767 || 32767 < v) return false;
    value = (int16_t)v;
    return true;
  }


  template <>
  bool String::parse<uint16_t>(const string &s, uint16_t &value, bool full) {
    uint32_t v;
    if (!parse<uint32_t>(s, v, full) || 65535 < v) return false;
    value = (uint16_t)v;
    return true;
  }


  template <>
  bool String::parse<int8_t>(const string &s, int8_t &value, bool full) {
    int32_t v;
    if (!parse<int32_t>(s, v, full) || v < -127 || 127 < v) return false;
    value = (int8_t)v;
    return true;
  }


  template <>
  bool String::parse<uint8_t>(const string &s, uint8_t &value, bool full) {
    uint32_t v;
    if (!parse<uint32_t>(s, v, full) || 255 < v) return false;
    value = (uint8_t)v;
    return true;
  }


  template <>
  bool String::parse<double>(const string &s, double &value, bool full) {
    errno = 0;
    char *end = 0;
    double v = strtod(s.c_str(), &end);
    if (errno || (full && end && *end)) return false;
    value = v;
    return true;
  }


  template <>
  bool String::parse<float>(const string &s, float &value, bool full) {
    errno = 0;
    char *end = 0;
    float v = strtof(s.c_str(), &end);
    if (errno || (full && end && *end)) return false;
    value = v;
    return false;
  }


  template <>
  bool String::parse<bool>(const string &s, bool &value, bool full) {
    string v = toLower(trim(s));

    if (v == "true" || v == "t" || v == "1" || v == "yes" || v == "y") {
      value = true;
      return true;
    }


    if (v == "false" || v == "f" || v == "0" || v == "no" || v == "n") {
      value = false;
      return true;
    }

    return false;
  }


#define CBANG_STRING_PT(NAME, TYPE, DESC)                               \
  template <>                                                           \
  TYPE String::parse<TYPE>(const string &s, bool full) {                \
    TYPE v = 0;                                                         \
    if (!parse<TYPE>(s, v, full))                                       \
      TYPE_ERROR("Invalid " DESC " value '" << s << "'");               \
    return v;                                                           \
  }
#include "StringParseTypes.def"
}


#define CBANG_STRING_PT(NAME, TYPE, DESC)                               \
  TYPE String::parse##NAME(const string &s, TYPE &value, bool full) {   \
    return parse<TYPE>(s, value, full);                                 \
  }
#include "StringParseTypes.def"


#define CBANG_STRING_PT(NAME, TYPE, DESC)                               \
  TYPE String::parse##NAME(const string &s, bool full) {                \
    return parse<TYPE>(s, full);                                        \
  }
#include "StringParseTypes.def"


#define CBANG_STRING_PT(NAME, TYPE, DESC)                       \
  TYPE String::is##NAME(const string &s, bool full) {           \
    TYPE v;                                                     \
    return parse##NAME(s, v, full);                             \
  }
#include "StringParseTypes.def"


bool String::isInteger(const string &s) {return isU64(s,    true);}
bool String::isNumber(const string &s)  {return isDouble(s, true);}


string String::trimLeft(const string &s, const string &delims) {
  string::size_type start = s.find_first_not_of(delims);

  if (start == string::npos) return "";
  return s.substr(start);
}


string String::trimRight(const string &s, const string &delims) {
  string::size_type end = s.find_last_not_of(delims);

  if (end == string::npos) return "";
  return s.substr(0, end + 1);
}


string String::trim(const string &s, const string &delims) {
  string::size_type start = s.find_first_not_of(delims);
  string::size_type end = s.find_last_not_of(delims);

  if (start == string::npos) return "";
  return s.substr(start, (end - start) + 1);
}


string String::toUpper(const string &s) {
  string::size_type len = s.length();
  string v(len, ' ');

  for (string::size_type i = 0; i < len; i++)
    v[i] = toupper(s[i]);

  return v;
}


string String::toLower(const string &s) {
  string::size_type len = s.length();
  string v(len, ' ');

  for (string::size_type i = 0; i < len; i++)
    v[i] = tolower(s[i]);

  return v;
}


string String::capitalize(const string &s) {
  string::size_type len = s.length();
  string v(len, ' ');

  bool whitespace = true;
  for (string::size_type i = 0; i < len; i++) {
    if (whitespace && isalpha(s[i])) v[i] = toupper(s[i]);
    else v[i] = s[i];
    whitespace = isspace(s[i]);
  }

  return v;
}


ostream &String::fill(ostream &stream, const string &str,
                      unsigned currentColumn, unsigned indent,
                      unsigned maxColumn) {
  unsigned pos = currentColumn;
  bool firstWord = true;
  const char *s = str.c_str();

  while (*s) {
    // Skip white space
    while (*s && *s != '\t' && isspace(*s)) {
      if (*s == '\n') {
        stream << '\n';
        pos = 0;
        firstWord = true;
      }

      s++;
    }
    if (!*s) break;

    // Tab in
    while (pos < indent) {
      stream << " ";
      pos++;
    }

    // Get word length
    unsigned len = 1;
    unsigned printLen = 1;
    while (s[len] && (s[len] == '\t' || !isspace(s[len]))) {
      if (s[len] == '\t') printLen++;
      printLen++;
      len++;
    }

    if (!firstWord && pos + printLen + 1 > maxColumn) {
      firstWord = true;
      stream << '\n';
      pos = 0;

    } else {
      if (!firstWord) {
        stream << ' ';
        pos++;
      }
      for (unsigned i = 0; i < len; i++)
        if (s[i] == '\t') stream.write("  ", 2);
        else stream.put(s[i]);

      firstWord = false;
      pos += printLen;
      s += len;
    }
  }

  return stream;
}


string String::fill(const string &str, unsigned currentColumn,
                    unsigned indent, unsigned maxColumn) {
  ostringstream stream;
  fill(stream, str, currentColumn, indent, maxColumn);
  return stream.str();
}


bool String::endsWith(const string &s, const string &part) {
  if (s.length() < part.length()) return false;

  return s.substr(s.length() - part.length()) == part;
}


bool String::startsWith(const string &s, const string &part) {
  if (s.length() < part.length()) return false;

  return s.substr(0, part.length()) == part;
}


string String::bar(const string &title, unsigned width, const string &chars) {
  if (width <= title.length()) return title;

  string result;

  if (title.length()) {
    // Add spaces around title
    result += title + " ";
    if (width <= result.length()) return result;
    result = string(" ") + result;
    if (width <= result.length()) return result;

    unsigned count = (width - result.length()) / 2;

    // Add left part
    string left;
    while (left.length() + chars.length() < count) left += chars;
    if (left.length() < count) left += chars.substr(0, count - left.length());
    result = left + result;
  }

  // Add right part
  while (result.length() + chars.length() < width) result += chars;
  if (result.length() < width)
    result += chars.substr(0, width = result.length());

  return result;
}


string String::hexdump(const char *data, unsigned size) {
  unsigned width = (unsigned)ceil(log((double)size) / log(2.0) / 4);
  string result;
  string chars;
  unsigned i;

  for (i = 0; i < size; i++) {
    if (i % 16 == 0) {
      if (i) {
        result += "  " + chars + '\n';
        chars.clear();
      }
      result += String::printf("0x%0*x", width, i);
    }

    if (i % 16 == 8) {
      result += ' ';
      chars += ' ';
    }

    result += String::printf(" %02x", (unsigned char)data[i]);
    switch (data[i]) {
    case '\a': chars.append("\\a"); break;
    case '\b': chars.append("\\b"); break;
    case '\f': chars.append("\\f"); break;
    case '\n': chars.append("\\n"); break;
    case '\r': chars.append("\\r"); break;
    case '\t': chars.append("\\t"); break;
    case '\v': chars.append("\\v"); break;
    default:
      if (0x19 < data[i] && data[i] < 0x7f) {
        chars.append(1, ' ');
        chars.append(1, data[i]);

      } else chars += " .";
      break;
    }
  }

  // Flush any remaining chars
  if (!chars.empty()) {
    for (; i % 16 != 0; i++) {
      if (i % 16 == 8) result += ' ';
      result.append("   ");
    }

    result += "  " + chars;
  }

  return result;
}


string String::hexdump(const uint8_t *data, unsigned size) {
  return hexdump((char *)data, size);
}


string String::hexdump(const string &s) {
#ifdef _WIN32
  return hexdump(s.c_str(), s.length());
#else
  return hexdump(s.data(), s.length());
#endif
}


char String::hexNibble(int x, bool lower) {
  x &= 0xf;
  return (x < 0xa ? '0' + x : (lower ? 'a' : 'A') + x - 0xa);
}


string String::hexEncode(const string &s) {
  string result;
  result.reserve(s.length() * 2);

  for (string::const_iterator it = s.begin(); it != s.end(); it++) {
    result.append(1, hexNibble(*it >> 4));
    result.append(1, hexNibble(*it));
  }

  return result;
}


string String::hexEncode(const char *data, unsigned length) {
  string result;
  result.reserve(length * 2);

  for (unsigned i = 0; i < length; i++) {
    result.append(1, hexNibble(data[i] >> 4));
    result.append(1, hexNibble(data[i]));
  }

  return result;
}


string String::escapeRE(const string &s) {
  static const Regex re("[\\^\\.\\$\\|\\(\\)\\[\\]\\*\\+\\?\\/\\\\]");
  static const string rep("\\\\\\1&");
  return re.replace(s, rep);
}


string String::escapeMySQL(const string &s) {
  string result;
  result.reserve(s.length());

  for (string::const_iterator it = s.begin(); it != s.end(); it++)
    switch (*it) {
    case 0:    result.append("\\0");  break;
    case '\'': result.append("\\'");  break;
    case '\"': result.append("\\\""); break;
    case '\b': result.append("\\b");  break;
    case '\n': result.append("\\n");  break;
    case '\r': result.append("\\r");  break;
    case '\t': result.append("\\t");  break;
    case 26:   result.append("\\Z");  break;
    case '\\': result.append("\\\\"); break;
    default: result.push_back(*it);   break;
    }

  return result;
}


void String::escapeC(string &result, char c) {
    switch (c) {
    case '\"': result.append("\\\""); break;
    case '\\': result.append("\\\\"); break;
    case '\a': result.append("\\a"); break;
    case '\b': result.append("\\b"); break;
    case '\f': result.append("\\f"); break;
    case '\n': result.append("\\n"); break;
    case '\r': result.append("\\r"); break;
    case '\t': result.append("\\t"); break;
    case '\v': result.append("\\v"); break;
    default: {
      uint8_t x = c;

      if (x < 0x20 || 0x7e < x) { // Non-printable
        if (x <= 0xff) result.append(printf("\\x%02x", x));
        else result.append(printf("\\u%04x", x));

      } else result.push_back(c);
      break;
    }
    }
}


string String::escapeC(char c) {
  string result;
  escapeC(result, c);
  return result;
}


string String::escapeC(const string &s) {
  string result;
  result.reserve(s.length());

  for (string::const_iterator it = s.begin(); it != s.end(); it++)
    escapeC(result, *it);

  return result;
}


namespace {
  bool is_oct(char c) {
    return '0' <= c && c <= '7';
  }


  bool is_hex(char c) {
    return
      ('a' <= c && c <= 'f') ||
      ('A' <= c && c <= 'F') ||
      ('0' <= c && c <= '9');
  }


  string::const_iterator parseOctalEscape(string &result,
                                          string::const_iterator start,
                                          string::const_iterator end) {
    string::const_iterator it = start + 1;

    string s;
    while (it != end && is_oct(*it) && s.length() < 3) s.push_back(*it++);

    if (s.empty()) return start;

    result.push_back((char)String::parseU8("0" + s));

    return it;
  }


  string::const_iterator parseHexEscape(string &result,
                                        string::const_iterator start,
                                        string::const_iterator end) {
    string::const_iterator it = start + 1;

    string s;
    while (it != end && is_hex(*it) && s.length() < 2) s.push_back(*it++);

    if (s.empty()) return start;

    result.push_back((char)String::parseU8("0x" + s));

    return it;
  }


  string::const_iterator parseUnicodeEscape(string &result,
                                            string::const_iterator start,
                                            string::const_iterator end) {
    string::const_iterator it = start + 1;

    string s;
    while (it != end && is_hex(*it) && s.length() < 4) s.push_back(*it++);

    if (s.length() != 4) return start;

    uint16_t code = String::parseU16("0x" + s);

    if (code < 0x80) result.push_back((char)code);

    else if (code < 0x800) {
      result.push_back(0xc0 | (code >> 6));
      result.push_back(0x80 | (code & 0x3f));

    } else {
      result.push_back(0xe0 | (code >> 12));
      result.push_back(0x80 | ((code >> 6) & 0x3f));
      result.push_back(0x80 | (code & 0x3f));
    }

    return it;
  }
}


string String::unescapeC(const string &s) {
  string result;
  result.reserve(s.length());

  bool escape = false;

  for (string::const_iterator it = s.begin(); it != s.end();) {
    if (escape) {
      escape = false;

      switch (*it) {
      case '0': it = parseOctalEscape(result, it, s.end()); continue;
      case 'a': result.push_back('\a'); break;
      case 'b': result.push_back('\b'); break;
      case 'f': result.push_back('\f'); break;
      case 'n': result.push_back('\n'); break;
      case 'r': result.push_back('\r'); break;
      case 't': result.push_back('\t'); break;
      case 'u': it = parseUnicodeEscape(result, it, s.end()); continue;
      case 'v': result.push_back('\v'); break;
      case 'x': it = parseHexEscape(result, it, s.end()); continue;
        // TODO handle \Uxxxxxxxx escapes
      default: result.push_back(*it); break;
      }

    } else if (*it == '\\') escape = true;

    else result.push_back(*it);

    it++;
  }

  return result;
}


string String::ellipsis(const string &s, unsigned width) {
  if (s.length() <= width) return s;
  return s.substr(0, width - 3) + "...";
}


size_t String::find(const string &s, const string &pattern,
                    vector<string> *groups) {
  Regex e(pattern);
  Regex::Match m;

  if (e.search(s, m)) {
    if (groups)
      for (unsigned i = 0; i < m.size(); i++)
        groups->push_back(m[i]);

    return m.position();
  }

  return string::npos;
}


string String::replace(const string &s, char search, char replace) {
  string result(s);

  for (string::iterator it = result.begin(); it != result.end(); it++)
    if (*it == search) *it = replace;

  return result;
}


string String::replace(const string &s, const string &search,
                       const string &replace) {
  Regex exp(search);
  return exp.replace(s, replace);
}


string String::transcode(const string &s, const string &search,
                         const string &replace) {
  if (search.length() != replace.length())
    THROW("Search string must be the same length as the replace string");

  string result(s.length(), ' ');

  unsigned i = 0;
  for (string::const_iterator it = s.begin(); it != s.end(); it++) {
    size_t pos = search.find(*it);
    if (pos != string::npos) result[i++] = replace[pos];
    else result[i++] = *it;
  }

  return result;
}


string String::format(format_cb_t cb) {
  string result;
  result.reserve(length());

  int index = 0;
  bool escape = false;

  for (string::const_iterator it = begin(); it != end(); it++) {
    if (escape) {
      escape  = false;

      switch (*it) {
      case '(': {
        string::const_iterator it2 = it + 1;

        string name;
        while (it2 != end() && *it2 != ')') name.push_back(*it2++);

        if (it2 != end() && ++it2 != end() && !name.empty()) {
          bool matched = true;
          string s = cb(*it2, -1, name, matched);

          if (matched) {
            result.append(s);
            it = it2;
            continue;
          }
        }

        result.push_back('%');
        break;
      }

      case '%': break;

      default: {
        bool matched = true;
        string s = cb(*it, index++, "", matched);

        if (matched) {
          result.append(s);
          continue;
        }

        result.push_back('%');
      }
      }

    } else if (*it == '%') {
      escape = true;
      continue;
    }

    result.push_back(*it);
  }

  if (escape) result.push_back('%');

  return result;
}


string String::makeFormatString(char type, const string &name) {
  if (name.empty()) return "%" + string(1, type);
  return "%(" + name + ")" + string(1, type);
}
