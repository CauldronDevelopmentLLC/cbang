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

#include "Info.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/xml/Writer.h>
#include <cbang/json/JSON.h>

#include <iomanip>

using namespace std;
using namespace cb;


Info::category_t &Info::add(const string &category, bool prepend) {
  auto it = categories.find(category);
  if (it == categories.end())
    it = categories.insert(category, category_t(), prepend);
  return it.value();
}


void Info::add(const string &category, const string &key, const string &value,
               bool prepend) {
  category_t &cat = add(category, prepend);
  cat.insert(key, value, prepend);
  if (maxKeyLength < key.length()) maxKeyLength = key.length();
}


const string &Info::get(const string &category, const string &key) const {
  auto it = categories.find(category);
  if (it == categories.end())
    THROW("Info category '" << category << "' does not exist.");

  const category_t &cat = it.value();

  auto it2 = cat.find(key);
  if (it2 == cat.end())
    THROW("Info category '" << category << "' does have key '" << key << "'.");

  return it2.value();
}


bool Info::has(const string &category, const string &key) const {
  auto it = categories.find(category);
  if (it == categories.end()) return false;

  const category_t &cat = it.value();

  return cat.find(key) != cat.end();
}


ostream &Info::print(ostream &stream, unsigned width, bool wrap) const {
  for (auto it: categories) {
    if (it.key() != "") stream << String::bar(it.key(), width) << '\n';

    for (auto it2: it.value()) {
      if (it2.value().empty()) continue; // Don't print empty values

      stream << setw(maxKeyLength) << it2.key() << ": ";
      if (wrap)
        String::fill(stream, it2.value(), maxKeyLength + 2, maxKeyLength + 2);
      else stream << it2.value();
      stream << '\n';
    }
  }

  stream << String::bar("", width) << '\n';

  return stream;
}


void Info::write(XML::Writer &writer) const {
  XML::Attributes attrs;
  attrs["class"] = "info";
  writer.startElement("table", attrs);

  for (auto it: categories) {
    if (it.key() != "") {
      writer.startElement("tr");

      XML::Attributes attrs;
      attrs["colspan"] = "2";
      attrs["class"] = "category";
      writer.startElement("th", attrs);
      writer.text(it.key());
      writer.endElement("th");

      writer.endElement("tr");
    }

    for (auto it2: it.value()) {
      if (it2.value().empty()) continue;
      writer.startElement("tr");
      writer.startElement("th");
      writer.text(it2.key());
      writer.endElement("th");
      writer.startElement("td");
      writer.text(it2.value());
      writer.endElement("td");
      writer.endElement("tr");
    }
  }

  writer.endElement("table");
}


void Info::writeList(JSON::Sink &sink) const {
  sink.beginList();

  for (auto it: categories) {
    sink.appendList();
    sink.append(it.key());

    for (auto &it2: it.value()) {
      if (it2.value().empty()) continue;
      sink.appendList(true);
      sink.append(it2.key());
      sink.append(it2.value());
      sink.endList();
    }

    sink.endList();
  }

  sink.endList();
}


void Info::write(JSON::Sink &sink) const {
  sink.beginDict();

  for (auto it: categories) {
    sink.insertDict(it.key());

    for (auto it2: it.value())
      sink.insert(it2.key(), it2.value());

    sink.endDict();
  }

  sink.endDict();
}
