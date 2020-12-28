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

#include "Reader.h"
#include "Builder.h"

#include <cbang/String.h>
#include <cbang/io/StringInputSource.h>

#include <vector>
#include <cctype>
#include <sstream>

#include <errno.h>

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


SmartPointer<Value> Reader::parse(const InputSource &src) {
  return Reader(src).parse();
}


SmartPointer<Value> Reader::parseString(const string &s) {
  return parse(StringInputSource(s));
}


void Reader::parse(const InputSource &src, Sink &sink) {
  Reader(src).parse(sink);
}


void Reader::parseString(const string &s, Sink &sink) {
  parse(StringInputSource(s), sink);
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

    if (!errno && (size_t)(end - start) == value.length()) {
      sink.write((int64_t)v);
      return;
    }

  } else if (!decimal) {
    long long unsigned v = strtoull(start, &end, 0);

    if (!errno && (size_t)(end - start) == value.length()) {
      sink.write((uint64_t)v);
      return;
    }
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
      case 'f': s += '\b'; break;
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
        uint16_t code = 0;

        for (unsigned i = 0; i < 4; i++) {
          code <<= 4;
          c = get();

          if ('0' <= c && c <= '9') code += c - '0';
          else if ('a' <= c && c <= 'f') code += c - 'a' + 10;
          else if ('A' <= c && c <= 'F') code += c - 'A' + 10;
          else error("Invalid unicode escape sequence in JSON");
        }

        if (code < 0x80) s += (char)code;
        else if (code < 0x800) {
          s += 0xc0 | (code >> 6);
          s += 0x80 | (code & 0x3f);

        } else {
          s += 0xe0 | (code >> 12);
          s += 0x80 | ((code >> 6) & 0x3f);
          s += 0x80 | (code & 0x3f);
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
      // Compute code width
      unsigned width;
      if ((c & 0xe0) == 0xc0) width = 1;
      else if ((c & 0xf0) == 0xe0) width = 2;
      else if ((c & 0xf8) == 0xf0) width = 3;
      else error(SSTR("Invalid UTF-8 byte '" <<
                      String::printf("0x%02x", (unsigned)c)
                      << " in JSON string"));

      s += c;
      for (unsigned i = 0; i < width; i++) {
        c = get();
        if ((c & 0xc0) != 0x80)
          error("Incomplete UTF-8 sequence in JSON string");
        s += 3;
      }

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


string Reader::unescape(const string &s) {return cb::String::unescapeC(s);}
