/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include "Value.h"
#include "Path.h"

#include <sstream>
#include <limits>

using namespace std;
using namespace cb;
using namespace cb::JSON;


bool Value::exists(const string &path) const {return Path(path).exists(*this);}


ValuePtr Value::select(const string &path) const {
  return Path(path).select(*this);
}


ValuePtr Value::select(const string &path, const ValuePtr &defaultValue) const {
  return Path(path).select(*this, defaultValue);
}


void Value::appendFrom(const Value &value) {for (auto &v: value) append(v);}


void Value::merge(const Value &value) {
  if (this == &value) return; // They are the same object

  // Merge lists
  if (isList() && value.isList()) {
    appendFrom(value);
    return;
  }

  if (!isDict() || !value.isDict())
    TYPE_ERROR("Cannot merge JSON nodes of type " << getType() << " and "
               << value.getType());

  // Merge dicts
  for (auto e: value.entries()) {
    if (has(e.key())) {
      ValuePtr dst = get(e.key());

      if ((e.value()->isDict() && dst->isDict()) ||
          (e.value()->isList() && dst->isList())) {
        dst->merge(*e.value());
        continue;
      }
    }

    insert(e.key(), e.value());
  }
}


string Value::format(char type) const {
  switch (type) {
  case 'b': return String(getBoolean());
  case 'f': return String(getNumber());
  case 'i': return String(getS32());
  case 'u': return String(getU32());
  case 's': return asString();
  case 'S': return "\"" + String::escapeMySQL(asString()) + "\"";
  }

  THROW("Unsupported format type specifier '"
        << String::escapeC(string(1, type)) << "'");
}


string Value::format(char type, int index, const string &name,
                     bool &matched) const {
  if (index < 0) {
    if (exists(name)) return select(name)->format(type);
    if (type == 'b')  return String(false);
  } else if ((unsigned)index < size()) return get(index)->format(type);

  matched = false;
  return "";
}


string Value::format(const string &s, const string &defaultValue) const {
  auto cb =
    [&] (char type, int index, const string &name, bool &matched) {
      string result = format(type, index, name, matched);
      if (matched) return result;
      matched = true;
      return defaultValue;
    };

  return String(s).format(cb);
}


string Value::format(const string &s) const {
  auto cb =
    [&] (char type, int index, const string &name, bool &matched) {
      return format(type, index, name, matched);
    };

  return String(s).format(cb);
}


void Value::visit(const_visitor_t visitor, bool depthFirst) const {
  if (!depthFirst) visitor(*this, 0);
  visitChildren(visitor, depthFirst);
  if (depthFirst) visitor(*this, 0);
}


void Value::visit(visitor_t visitor, bool depthFirst) {
  if (!depthFirst) visitor(*this, 0);
  visitChildren(visitor, depthFirst);
  if (depthFirst) visitor(*this, 0);
}


void Value::write(ostream &stream, unsigned indentStart, bool compact,
                  unsigned indentSpace, int precision) const {
  Writer writer(stream, indentStart, compact, indentSpace, precision);
  write(writer);
}


string Value::toString(unsigned indentStart, bool compact, unsigned indentSpace,
                       int precision) const {
  ostringstream str;
  write(str, indentStart, compact, indentSpace, precision);
  str << flush;
  return str.str();
}


string Value::asString() const {return isString() ? getString() : toString();}


bool Value::operator==(const Value &o) const {
  if (getType() != o.getType()) return false;

  switch (getType()) {
  case JSON_NULL: case JSON_UNDEFINED: return true;
  case JSON_BOOLEAN: return getBoolean() == o.getBoolean();
  case JSON_STRING:  return getString()  == o.getString();
    // TODO Some numbers are not represented exactly as doubles
  case JSON_NUMBER:  return getNumber()  == o.getNumber();

  case JSON_LIST:
    if (size() != o.size()) return false;

    for (unsigned i = 0; i < size(); i++)
      if (*get(i) != *o.get(i)) return false;

    return true;

  case JSON_DICT:
    if (size() != o.size()) return false;

    for (auto e: entries())
      if (!o.has(e.key()) || *e.value() != *o.get(e.key()))
        return false;

    return true;
  }

  return false;
}
