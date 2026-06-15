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

#include "Reader.h"
#include "Builder.h"

#include <cbang/String.h>
#include <cbang/log/Logger.h>

#include <vector>
#include <cctype>
#include <sstream>
#include <cerrno>

using namespace std;
using namespace cb;
using namespace cb::JSON;


void Reader::parse(Sink &sink, unsigned depth) {
  if (1000 < ++depth) error("Maximum JSON parse depth reached");

  while (good()) {
    switch (next()) {
    case 'N': case 'n':
      parseNull();
      return sink.writeNull();

    case 'T': case 'F': case 't': case 'f':
      return sink.writeBoolean(parseBoolean());

    case '-': case '.':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      parseNumber(sink);
      return;

    case '"':
      return sink.write(parseString());

    case '[':
      sink.beginList();
      parseList(sink, depth);
      sink.endList();
      return;

    case '{':
      sink.beginDict();
      parseDict(sink, depth);
      sink.endDict();
      return;

    default: match("NnTtFf-.0123456789\"[{");
    }
  }

  error("Unexpected end of expression");
  throw "Unreachable";
}


ValuePtr Reader::parse() {
  Builder builder;
  parse(builder);
  return builder.getRoot();
}


SmartPointer<Value> Reader::parse(const InputSource &src, bool strict) {
  return Reader(src, strict).parse();
}


SmartPointer<Value> Reader::parseFile(const string &path, bool strict) {
  return parse(InputSource::open(path), strict);
}


void Reader::parse(const InputSource &src, Sink &sink, bool strict) {
  Reader(src, strict).parse(sink);
}


void Reader::parseFile(const string &path, Sink &sink, bool strict) {
  parse(InputSource::open(path), sink, strict);
}


char Reader::get() {
  char c = stream.get();

  if (c == '\n') {
    line++;
    column = 0;

  } else if (c != '\r') column++;

  return c;
}


char Reader::next() {
  while (good())
    switch (stream.peek()) {
    case '\n': case '\r': case '\t': case ' ': get(); break;

    case '#':
      while (stream.peek() != '\n') get();
      break;

    default: return stream.peek();
    }

  error("Unexpected end of expression");
  throw "Unreachable";
}


bool Reader::tryMatch(char c) {
  if (c == next()) {
    advance();
    return true;
  }

  return false;
}


char Reader::match(const char *chars) {
  char x = next();

  for (int i = 0; chars[i]; i++)
    if (x == chars[i]) {
      advance();
      return x;
    }

  error(SSTR("Expected one of '" << cb::String::escapeC(chars)
             << "' but found '" << cb::String::escapeC(string(1, x)) << '\''));
  throw "Unreachable";
}


const string Reader::parseKeyword() {
  string s;
  while (good() && isalpha(stream.peek())) s += stream.get();
  return s;
}


void Reader::parseNull() {
  if (strict) {
    string value = parseKeyword();
    if (value != "null") error(SSTR("'null' but found '" << value << '\''));

  } else {
    string value = cb::String::toLower(parseKeyword());

    if (value != "none" && value != "null")
      error(SSTR("Expected keyword 'None' or 'null' but found '" << value
                 << '\''));
  }
}


bool Reader::parseBoolean() {
  string value = parseKeyword();
  if (!strict) value = cb::String::toLower(value);

  if (value == "true") return true;
  else if (value == "false") return false;

  error(SSTR("Expected keyword 'true' or 'false' but found '" << value << "'"));
  throw "Unreachable";
}


void Reader::parseNumber(Sink &sink) {
  string value;
  bool negative = false;
  bool decimal = false;

  // NOTE, we use next() to skip leading whitespace but no whitespace is allowed
  // with in the number itself so stream.get() is called directly.
  if (next() == '-') {
    value += get();
    negative = true;
  }

  if (stream.peek() == '0') value += stream.get();
  else {
    if (strict && !isdigit(stream.peek()))
      error("Missing digit at start of number");
    while (isdigit(stream.peek())) value += stream.get();
  }

  if (stream.peek() == '.') {
    decimal = true;
    value += stream.get();
    if (strict && !isdigit(stream.peek()))
      error("Missing digit after decimal point");
    while (isdigit(stream.peek())) value += stream.get();
  }

  if (stream.peek() == 'e' || stream.peek() == 'E') {
    decimal = true;
    value += stream.get();
    if (stream.peek() == '+' || stream.peek() == '-') value += stream.get();
    if (strict && !isdigit(stream.peek()))
      error("Missing digit in exponent");
    while (isdigit(stream.peek())) value += stream.get();
  }

  const char *start = value.c_str();
  char *end;
  errno = 0;

  if (!decimal && negative) {
    long long int v = strtoll(start, &end, 0);

    if (!errno && (size_t)(end - start) == value.length())
      return sink.write((int64_t)v);

  } else if (!decimal) {
    long long unsigned v = strtoull(start, &end, 0);

    if (!errno && (size_t)(end - start) == value.length())
      return sink.write((uint64_t)v);
  }

  double v = strtod(start, &end);
  if (errno || (size_t)(end - start) != value.length())
    error(SSTR("Invalid JSON number '" << value << "'"));
  sink.write(v);
}


string Reader::parseString() {
  match("\"");

  unsigned char c = 0;
  string s;
  bool escape = false;
  while (good()) {
    c = get();
    if (!good()) break;

    if (c == '\n') error("Unescaped new line in JSON string");

    if (escape) {
      escape = false;

      switch (c) {
      case '"': case '\\': case '/': s += c; break;
      case 'b': s += '\b'; break;
      case 'f': s += '\f'; break;
      case 'n': s += '\n'; break;
      case 'r': s += '\r'; break;
      case 't': s += '\t'; break;

      case 'x': {
        if (strict) error("Hex escape sequence not allowed in JSON");

        uint16_t code = 0;
        for (unsigned i = 0; i < 2; i++) {
          code <<= 4;
          c = get();
          if ('0' <= c && c <= '9') code += c - '0';
          else if ('a' <= c && c <= 'f') code += c - 'a' + 10;
          else if ('A' <= c && c <= 'F') code += c - 'F' + 10;
          else error(SSTR("Invalid hex character '" << String::escapeC(c)
                          << "' in JSON string"));
        }

        s += code;
        break;
      }

      case 'u': {
        auto readHex4 = [&] () {
          uint32_t code = 0;

          for (unsigned i = 0; i < 4; i++) {
            code <<= 4;
            char h = get();

            if      ('0' <= h && h <= '9') code += h - '0';
            else if ('a' <= h && h <= 'f') code += h - 'a' + 10;
            else if ('A' <= h && h <= 'F') code += h - 'A' + 10;
            else error("Invalid unicode escape sequence in JSON");
          }

          return code;
        };

        uint32_t code = readHex4();

        // Characters above U+FFFF (e.g. emoji) are escaped as a UTF-16
        // surrogate pair: a high surrogate followed by a low surrogate.
        // They must be combined into one code point; emitting each half
        // on its own would produce invalid UTF-8 (a 0xD800-0xDFFF code
        // point) that downstream consumers reject.
        if (0xd800 <= code && code <= 0xdbff) {
          if (get() != '\\' || get() != 'u')
            error("Expected low surrogate in JSON unicode escape");

          uint32_t low = readHex4();
          if (low < 0xdc00 || 0xdfff < low)
            error("Invalid low surrogate in JSON unicode escape");

          code = 0x10000 + ((code - 0xd800) << 10) + (low - 0xdc00);

        } else if (0xdc00 <= code && code <= 0xdfff)
          error("Unexpected low surrogate in JSON unicode escape");

        if (code < 0x80) s += (char)code;
        else if (code < 0x800) {
          s += (char)(0xc0 | (code >> 6));
          s += (char)(0x80 | (code & 0x3f));

        } else if (code < 0x10000) {
          s += (char)(0xe0 | (code >> 12));
          s += (char)(0x80 | ((code >> 6) & 0x3f));
          s += (char)(0x80 | (code & 0x3f));

        } else {
          s += (char)(0xf0 | (code >> 18));
          s += (char)(0x80 | ((code >> 12) & 0x3f));
          s += (char)(0x80 | ((code >> 6) & 0x3f));
          s += (char)(0x80 | (code & 0x3f));
        }

        break;
      }

      default:
        if ('0' <= c && c <= '7') {
          if (strict) error("Hex escape sequence not allowed in JSON");

          uint16_t code = 0;
          for (unsigned i = 0; i < 3; i++) {
            code <<= 3;
            if (i) c = get();
            if ('0' <= c && c <= '7') code += c - '0';
            else error(SSTR("Invalid octal character '" << String::escapeC(c)
                            << "' in JSON string"));
          }

          if (0377 < code) error("Invalid octal code in JSON string");
          s += code;

        } else error(SSTR("Invalid string escape character '"
                          << String::escapeC(c) << "' in JSON"));
      }

    } else if (c == '"') break;
    else if (c == '\\') escape = true;
    else if (0x00 <= c && c <= 0x1f)
      error("Control characters not allowed in JSON strings");

    else if (0x80 <= c) {
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

      // Compute code width and the bits the lead byte contributes
      unsigned width = 0;
      uint32_t code = 0;
      if      ((c & 0xe0) == 0xc0) {width = 1; code = c & 0x1f;}
      else if ((c & 0xf0) == 0xe0) {width = 2; code = c & 0x0f;}
      else if ((c & 0xf8) == 0xf0) {width = 3; code = c & 0x07;}
      else error(SSTR("Invalid UTF-8 byte '" <<
                      String::printf("0x%02x", (unsigned)c)
                      << " in JSON string"));

      s += c;
      for (unsigned i = 0; i < width; i++) {
        c = get();
        if ((c & 0xc0) != 0x80)
          error("Incomplete UTF-8 sequence in JSON string");
        code = (code << 6) | (c & 0x3f);
        s += c;
      }

      // Reject overlong encodings, UTF-16 surrogates and code points
      // beyond U+10FFFF, none of which are valid UTF-8 (RFC 3629)
      static const uint32_t minCode[] = {0x80, 0x800, 0x10000};
      if (code < minCode[width - 1])
        error("Overlong UTF-8 encoding in JSON string");
      if (0xd800 <= code && code <= 0xdfff)
        error("UTF-8 encoded surrogate in JSON string");
      if (0x10ffff < code)
        error("UTF-8 code point beyond U+10FFFF in JSON string");

    } else s += c;
  }

  if (c != '"') error("Unclosed string in JSON");

  return s;
}


void Reader::parseList(Sink &sink, unsigned depth) {
  match("[");

  bool comma = false;

  while (good()) {
    if (tryMatch(']')) {
      // Empty list or trailing comma
      if (strict && comma) error("Trailing comma not allowed in JSON list");
      return;
    }

    sink.beginAppend();
    parse(sink, depth);

    if (match(",]") == ']') return; // Continuation or end
    comma = true;
  }
}


void Reader::parseDict(Sink &sink, unsigned depth) {
  match("{");

  bool comma = false;

  while (good()) {
    if (tryMatch('}')) {
      // Empty dict or trailing comma
      if (strict && comma) error("Trailing comma not allowed in JSON dict");
      return;
    }

    string key = parseString();
    match(":");
    sink.beginInsert(key);
    parse(sink, depth);

    if (match(",}") == '}') return; // Continuation or end
    comma = true;
  }
}


void Reader::error(const string &msg) const {
  throw ParseError(msg, FileLocation(src.getName(), line, column));
}
