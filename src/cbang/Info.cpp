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

#include "Info.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/xml/Writer.h>
#include <cbang/json/JSON.h>

#include <iomanip>

using namespace std;
using namespace cb;


Info::category_t &Info::add(const string &category, bool prepend) {
  auto result =
    categories.insert(categories_t::value_type(category, category_t()));
  category_t &cat = result.first->second;

  if (result.second) {
    if (prepend) categories.push_front(&*result.first);
    else categories.push_back(&*result.first);
  }

  return cat;
}


void Info::add(const string &category, const string &key, const string &value,
               bool prepend) {
  category_t &cat = add(category, prepend);
  auto result = cat.insert(category_t::value_type(key, value));

  if (result.second) {
    if (prepend) cat.push_front(&*result.first);
    else cat.push_back(&*result.first);

  } else result.first->second = value;

  if (maxKeyLength < key.length()) maxKeyLength = key.length();
}


const string &Info::get(const string &category, const string &key) const {
  auto it = categories.find(category);
  if (it == categories.map_t::end())
    THROW("Info category '" << category << "' does not exist.");

  const category_t &cat = it->second;

  auto it2 = cat.find(key);
  if (it2 == cat.map_t::end())
    THROW("Info category '" << category << "' does have key '" << key << "'.");

  return it2->second;
}


bool Info::has(const string &category, const string &key) const {
  auto it = categories.find(category);
  if (it == categories.map_t::end()) return false;

  const category_t &cat = it->second;

  return cat.find(key) != cat.map_t::end();
}


ostream &Info::print(ostream &stream, unsigned width, bool wrap) const {
  for (auto ptr: categories) {
    if (ptr->first != "") stream << String::bar(ptr->first, width) << '\n';

    for (auto ptr2: ptr->second) {
      if (ptr2->second.empty()) continue; // Don't print empty values

      stream << setw(maxKeyLength) << ptr2->first << ": ";
      if (wrap)
        String::fill(stream, ptr2->second, maxKeyLength + 2, maxKeyLength + 2);
      else stream << ptr2->second;
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

  for (auto ptr: categories) {
    if (ptr->first != "") {
      writer.startElement("tr");

      XML::Attributes attrs;
      attrs["colspan"] = "2";
      attrs["class"] = "category";
      writer.startElement("th", attrs);
      writer.text(ptr->first);
      writer.endElement("th");

      writer.endElement("tr");
    }

    for (auto ptr2: ptr->second) {
      if (ptr2->second.empty()) continue;
      writer.startElement("tr");
      writer.startElement("th");
      writer.text(ptr2->first);
      writer.endElement("th");
      writer.startElement("td");
      writer.text(ptr2->second);
      writer.endElement("td");
      writer.endElement("tr");
    }
  }

  writer.endElement("table");
}


void Info::writeList(JSON::Sink &sink) const {
  sink.beginList();

  for (auto ptr: categories) {
    sink.appendList();
    sink.append(ptr->first);

    for (auto ptr2: ptr->second) {
      if (ptr2->second.empty()) continue;
      sink.appendList(true);
      sink.append(ptr2->first);
      sink.append(ptr2->second);
      sink.endList();
    }

    sink.endList();
  }

  sink.endList();
}


void Info::write(JSON::Sink &sink) const {
  sink.beginDict();

  for (auto ptr: categories) {
    sink.insertDict(ptr->first);

    for (auto ptr2: ptr->second)
      sink.insert(ptr2->first, ptr2->second);

    sink.endDict();
  }

  sink.endDict();
}
