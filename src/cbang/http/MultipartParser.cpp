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

#include "MultipartParser.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


namespace {
  // Value of parameter `key` (lower-case) in a `;`-separated header parameter
  // list, honoring quoted values (which may contain ';' or escaped quotes).
  // Returns "" if the key is absent.
  string parseParam(const string &s, const string &key) {
    size_t i = 0, n = s.length();
    while (i < n && s[i] != ';') i++;  // Skip the leading type / value

    while (i < n) {
      if (s[i] == ';') i++;
      while (i < n && (s[i] == ' ' || s[i] == '\t')) i++;

      size_t start = i;                      // Parameter name
      while (i < n && s[i] != '=' && s[i] != ';') i++;
      string name = String::toLower(String::trim(s.substr(start, i - start)));

      string value;
      if (i < n && s[i] == '=') {
        i++;
        if (i < n && s[i] == '"') {          // Quoted value
          for (i++; i < n && s[i] != '"'; i++) {
            if (s[i] == '\\' && i + 1 < n) i++;  // Unescape quoted-pair
            value += s[i];
          }
          if (i < n) i++;                    // Skip the closing quote
        } else {                             // Bare token
          start = i;
          while (i < n && s[i] != ';') i++;
          value = String::trim(s.substr(start, i - start));
        }
      }

      if (name == key) return value;
    }

    return "";
  }


  // Parse one part's headers, filling name/filename/type.
  void parseHeaders(const string &headers, MultipartParser::Part &part) {
    vector<string> lines;
    String::tokenize(headers, lines, "\r\n");

    for (auto &line: lines) {
      size_t colon = line.find(':');
      if (colon == string::npos) continue;

      string name  = String::toLower(String::trim(line.substr(0, colon)));
      string value = String::trim(line.substr(colon + 1));

      if (name == "content-disposition") {
        part.name     = parseParam(value, "name");
        part.filename = parseParam(value, "filename");

      } else if (name == "content-type") part.type = value;
    }
  }
}


string MultipartParser::getBoundary(const string &contentType) {
  if (!String::startsWith(String::toLower(contentType), "multipart/form-data"))
    return "";
  return parseParam(contentType, "boundary");
}


vector<MultipartParser::Part> MultipartParser::parse(
  const string &body, const string &boundary) {
  if (boundary.empty()) THROW("Empty multipart boundary");

  vector<Part> parts;
  string dashBoundary = "--" + boundary;
  string delim        = "\r\n" + dashBoundary;

  // Locate the first delimiter, which must begin a line (RFC 7578): at the
  // very start of the body, else the first CRLF-anchored one (skipping any
  // preamble).  A bare substring search would match the boundary token mid-
  // line in the preamble and reject a valid body.
  size_t pos;
  if (body.compare(0, dashBoundary.length(), dashBoundary) == 0)
    pos = dashBoundary.length();
  else {
    pos = body.find(delim);
    if (pos == string::npos) THROW("Multipart boundary not found");
    pos += delim.length();
  }

  while (true) {
    // A closing delimiter ("--boundary--") ends the body.
    if (body.compare(pos, 2, "--") == 0) break;

    // Otherwise skip optional whitespace and the required CRLF.
    while (pos < body.length() && (body[pos] == ' ' || body[pos] == '\t'))
      pos++;
    if (body.compare(pos, 2, "\r\n")) THROW("Malformed multipart boundary");
    pos += 2;

    // Headers run up to a blank line.
    size_t headersEnd = body.find("\r\n\r\n", pos);
    if (headersEnd == string::npos) THROW("Unterminated multipart headers");

    // Data runs up to the next delimiter.
    size_t dataStart = headersEnd + 4;
    size_t dataEnd   = body.find(delim, dataStart);
    if (dataEnd == string::npos) THROW("Unterminated multipart part");

    Part part;
    parseHeaders(body.substr(pos, headersEnd - pos), part);
    part.data = body.substr(dataStart, dataEnd - dataStart);
    parts.push_back(part);

    pos = dataEnd + delim.length();
  }

  return parts;
}
