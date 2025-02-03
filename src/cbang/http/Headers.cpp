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

#include "Headers.h"
#include "ContentTypes.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/Errors.h>
#include <cbang/event/Buffer.h>

using namespace cb::HTTP;
using namespace std;


bool Headers::has(const string &key) const {
  return HeadersImpl::find(key) != end();
}


string Headers::find(const string &key) const {
  auto it = HeadersImpl::find(key);
  return it == end() ? "" : it.value();
}


const string &Headers::get(const string &key) const {
  auto it = HeadersImpl::find(key);
  if (it == end()) CBANG_KEY_ERROR("Header '" << key << "' not found");
  return it.value();
}


void Headers::remove(const string &key) {erase(key);}


bool Headers::keyContains(const string &key, const string &value) const{
  string hdr = String::toLower(find(key));
  vector<string> parts;

  String::tokenize(hdr, parts, " ,");

  for (auto &part: parts)
    if (part == String::toLower(value)) return true;

  return false;
}


string Headers::getContentType() const {return find("Content-Type");}


bool Headers::isJSONContentType() const {
  return String::startsWith(getContentType(), "application/json");
}


void Headers::setContentType(const string &contentType) {
  insert("Content-Type", contentType);
}


void Headers::guessContentType(const string &ext) {
  setContentType(ContentTypes::guess(ext, "text/html; charset=UTF-8"));
}


/// @return true if we should send a "Connection: close" when request done.
bool Headers::needsClose() const {return keyContains("Connection", "close");}
bool Headers::connectionKeepAlive() const {
  return keyContains("Connection", "keep-alive");
}


bool Headers::parse(Event::Buffer &buf, unsigned maxSize) {
  unsigned bytes = 0;
  string last;

  while (buf.getLength()) {
    string line;
    if (!buf.readLine(line, maxSize ? maxSize - bytes : 0)) return false;

    // Last header
    if (line.empty()) return true;

    bytes += line.length() + 2;
    if (maxSize && maxSize < bytes) THROW("Header too long");

    // Continuation line
    if (line[0] == ' ' || line[0] == '\t') {
      auto it = HeadersImpl::find(last);
      if (it == HeadersImpl::end()) THROW("Invalid header line: " << line);
      it.value() += String::trim(line);
      continue;
    }

    // Parse
    size_t semi = line.find_first_of(':');
    if (semi == string::npos) THROW("Invalid header line: " << line);

    string key = line.substr(0, semi);
    string value = String::trim(line.substr(semi + 1));

    // See RFC 2616 Section 4.2 "Message Headers"
    auto it = HeadersImpl::find(key);
    if (it != end()) {
      auto &h = it.value();
      if (!String::trim(h).empty()) h += ", ";
      h += value;

    } else insert(key, value);

    last = key;
  }

  return false;
}


void Headers::write(ostream &stream) const {
  for (auto it: *this)
    stream << it.key() << ": " << it.value() << '\n';
}
