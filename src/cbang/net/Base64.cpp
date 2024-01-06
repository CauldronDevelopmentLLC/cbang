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

#include "Base64.h"

#include <cbang/Exception.h>

#include <locale>
#include <utility>  // std::move()
#include <cstring> // memcpy()

using namespace std;
using namespace cb;


namespace {
  char next(string::const_iterator &it, string::const_iterator end) {
    char c = *it++;
    while (it != end && isspace(*it)) it++;
    return c;
  }
}


const char *Base64::_encodeTable =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

const signed char Base64::_decodeTable[256] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
  -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
  -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};


Base64::Base64(unsigned width) : width(width) {
  memcpy(encodeTable, _encodeTable, 65);
  memcpy(decodeTable, _decodeTable, 256);
}


Base64::Base64(char pad, char a, char b, unsigned width) : Base64(width) {
  encodeTable[62] = a;
  encodeTable[63] = b;
  encodeTable[64] = pad;
  decodeTable[(unsigned)a] = 62;
  decodeTable[(unsigned)b] = 63;
  if (pad) decodeTable[(unsigned)pad] = -2;
}


Base64::Base64(const char *pad, const char *a, const char *b, unsigned width) :
  Base64(*pad, *a, *b, width) {
  for (unsigned i = 1; pad[i]; i++) decodeTable[(unsigned)pad[i]] = -2;
  for (unsigned i = 1;   a[i]; i++) decodeTable[(unsigned)a[i]]   = 62;
  for (unsigned i = 1;   b[i]; i++) decodeTable[(unsigned)b[i]]   = 63;
}


string Base64::encode(const string &s) const {
  return encode(s.c_str(), s.length());
}


string Base64::encode(const char *_s, unsigned length) const {
  const uint8_t *s = (uint8_t *)_s;
  const uint8_t *end = s + length;

  struct Result : public string {
    unsigned col = 0;
    unsigned width;

    Result(unsigned length, unsigned width) : width(width) {
      unsigned size = length / 3 * 4 + 4; // Not exact
      if (width) size += (size / width) * 2;
      reserve(size);
    }

    void append(char c) {
      if (width) {
        if (col == width) {
          col = 1;
          string::append("\r\n");

        } else col++;
      }

      string::append(1, c);
    }
  };

  Result result(length, width);
  char pad = getPad();
  int padding = 0;
  uint8_t a, b, c;

  while (s != end) {
    a = *s++;
    if (s == end) {c = b = 0; padding = 2;}
    else {
      b = *s++;
      if (s == end) {c = 0; padding = 1;}
      else c = *s++;
    }

    result.append(encode(a >> 2));
    result.append(encode(a << 4 | b >> 4));
    if (padding == 2) {if (pad) result.append(pad);}
    else result.append(encode(b << 2 | c >> 6));
    if (padding) {if (pad) result.append(pad);}
    else result.append(encode(c));
  }

  return std::move(result);
}


string Base64::decode(const string &s) const {
  string result;
  result.reserve(s.length() / 4 * 3 + 1);

  string::const_iterator it = s.begin();
  while (it != s.end() && isspace(*it)) it++;

  while (it != s.end()) {
    char w = decode(next(it, s.end()));
    char x = it == s.end() ? -2 : decode(next(it, s.end()));
    char y = it == s.end() ? -2 : decode(next(it, s.end()));
    char z = it == s.end() ? -2 : decode(next(it, s.end()));

    if (w == -1 || w == -2 || x == -1 || x == -2 || y == -1 || z == -1)
      THROW("Invalid Base64 data at " << (it - s.begin()));

    result += (char)(w << 2 | x >> 4);
    if (y != -2) {
      result += (char)(x << 4 | y >> 2);
      if (z != -2) result += (char)(y << 6 | z);
    }
  }

  return result;
}


string Base64::decode(const char *s, unsigned length) const {
  return decode(string(s, length));
}


char Base64::getPad() const {return encodeTable[64];}
char Base64::encode(int x) const {return encodeTable[63 & x];}
int Base64::decode(char x) const {return decodeTable[(uint8_t)x];}
