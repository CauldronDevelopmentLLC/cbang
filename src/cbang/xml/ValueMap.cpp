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

#include "ValueMap.h"
#include "Reader.h"
#include "Writer.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/log/Logger.h>
#include <cbang/os/SystemUtilities.h>

using namespace std;
using namespace cb;
using namespace cb::XML;


ValueMap::ValueMap(const string &root) : root(root), depth(0) {}


void ValueMap::read(const string &filename) {
  setFilename(filename);
  read(*SystemUtilities::open(filename, ios::in));
}


void ValueMap::read(istream &stream) {
  Reader reader(false);
  reader.pushFile(filename);
  reader.read(stream, this);
  reader.popFile();
}


void ValueMap::write(const string &filename) const {
  write(*SystemUtilities::open(filename, ios::out | ios::trunc));
}


void ValueMap::write(ostream &stream) const {
  Writer writer(stream);

  writer.startElement(root);
  writeXMLValues(writer);
  writer.endElement(root);
}


void ValueMap::writeXMLValue(Writer &writer, const string &name,
                                const string &value) const {
  Attributes attrs;
  attrs["v"] = value;
  writer.startElement(name, attrs);
  writer.endElement(name);
}


void ValueMap::startElement(const string &name, const Attributes &attrs) {
  depth++;

  if (depth == 1) {
    if (name != root) THROW("Invalid root element '" << name << "'");
    return;

  } else if (depth > 2)
    THROW("Invalid child eleent '" << name << "' in XML value map");

  Attributes::const_iterator it;

  it = attrs.find("v");
  if (it == attrs.end()) it = attrs.find("value");

  if (it != attrs.end()) {
    setXMLValue(name, it->second);
    xmlValueSet = true;

  } else xmlValueSet = false;
}


void ValueMap::endElement(const string &name) {
  if (depth == 2 && !xmlValueSet) {
    setXMLValue(name, String::trim(xmlValue));
    xmlValue.clear();
  }

  depth--;
}


void ValueMap::text(const string &text) {
  xmlValue.append(text);
}


void ValueMap::cdata(const string &data) {
  xmlValue.append(data);
}
