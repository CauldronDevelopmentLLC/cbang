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

#include "Headers.h"
#include "Buffer.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/http/ContentTypes.h>

using namespace cb::Event;
using namespace std;


string Headers::find(const string &key) const {return has(key) ? get(key) : "";}
void Headers::remove(const std::string &key) {if (has(key)) insert(key, "");}


bool Headers::keyContains(const string &key, const string &value) const{
  string hdr = String::toLower(find(key));
  vector<string> parts;

  String::tokenize(hdr, parts, " ,");

  for (unsigned i = 0; i < parts.size(); i++)
    if (parts[i] == String::toLower(value)) return true;

  return false;
}


string Headers::getContentType() const {return find("Content-Type");}


void Headers::setContentType(const string &contentType) {
  insert("Content-Type", contentType);
}


void Headers::guessContentType(const std::string &ext) {
  HTTP::ContentTypes::const_iterator it =
    HTTP::ContentTypes::instance().find(String::toLower(ext));
  if (it != HTTP::ContentTypes::instance().end()) setContentType(it->second);
}


/// @return true if we should send a "Connection: close" when request done.
bool Headers::needsClose() const {return keyContains("Connection", "close");}
bool Headers::connectionKeepAlive() const {
  return keyContains("Connection", "keep-alive");
}


bool Headers::parse(Buffer &buf, unsigned maxSize) {
  unsigned bytes = 0;

  while (buf.getLength()) {
    string line;
    if (!buf.readLine(line, maxSize ? maxSize - bytes : 0)) return false;

    // Last header
    if (line.empty()) return true;

    bytes += line.length() + 2;
    if (maxSize && maxSize < bytes) THROW("Header too long");

    // Continuation line
    if (line[0] == ' ' || line[0] == '\t') {
      if (empty()) THROW("Invalid header line: " << line);
      get(size() - 1) += String::trim(line);
      continue;
    }

    // Parse
    size_t semi = line.find_first_of(':');
    if (semi == string::npos) THROW("Invalid header line: " << line);

    string key = line.substr(0, semi);
    string value = String::trim(line.substr(semi + 1));
    insert(key, value);
  }

  return false;
}


void Headers::write(ostream &stream) const {
  for (auto it = begin(); it != end(); it++)
    stream << it->first << ": " << it->second << '\n';
}
