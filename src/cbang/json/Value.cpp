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

#include "Value.h"
#include "Path.h"

#include <cbang/config.h>

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


string Value::formatAs(const string &spec) const {
  if (1 < spec.length()) THROW("Unsupported format spec '" << spec << "'");
  char type = spec.empty() ? 's' : spec[0];

  switch (type) {
  case 'b': return String(toBoolean());
  case 'u': return String(getU64());
  case 'd': return String(getS64());
  case 'f': return String(getNumber());
  case 's': return asString();
  default: THROW("Unsupported format type '" << type << "'");
  }
}


string Value::format(const string &fmt, const string &defaultValue) const {
  auto cb = [&] (const string &id, const string &spec) {
    auto result = select(id, 0);
    return result.isNull() ? defaultValue : result->formatAs(spec);
  };

  return String(fmt).format(cb);
}


string Value::format(const string &fmt) const {
  auto cb = [&] (const string &id, const string &spec) {
    auto result = select(id, 0);
    if (result.isNull()) CBANG_KEY_ERROR("Key '" << id << "' not found");
    return result->formatAs(spec);
  };

  return String(fmt).format(cb);
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


int Value::compare(const Value &o) const {
  if (getType() != o.getType()) return getType() - o.getType();

  switch (getType()) {
  case JSON_NULL: case JSON_UNDEFINED: return 0;
  case JSON_BOOLEAN: return getBoolean() - o.getBoolean();
  case JSON_STRING:  return getString().compare(o.getString());
  case JSON_NUMBER: {
    // TODO Some numbers are not represented exactly as doubles
    auto a = getNumber();
    auto b = o.getNumber();
    return a < b ? -1 : (b < a ? 1 : 0);
  }

  case JSON_LIST:
    if (size() != o.size()) return size() - o.size();

    for (unsigned i = 0; i < size(); i++) {
      int ret = get(i)->compare(*o.get(i));
      if (ret) return ret;
    }

    return 0;

  case JSON_DICT: {
    if (size() != o.size()) return size() - o.size();

    auto itA = entries().begin();
    auto itB = o.entries().begin();

    for (unsigned i = 0; i < size(); i++) {
      int ret = itA.key().compare(itB.key());
      if (ret) return ret;
      ret = itA.value()->compare(*itB.value());
      if (ret) return ret;

      itA++;
      itB++;
    }

    return 0;
  }
  }

  THROW("Unexpected JSON type: " << getType());
}
