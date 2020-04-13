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

#include "Value.h"
#include "Path.h"

#include <sstream>
#include <limits>

using namespace std;
using namespace cb;
using namespace cb::JSON;


ConstValuePtr Value::select(const string &path) const {
  return Path(path).select(*this);
}


ConstValuePtr Value::select(const string &path,
                            const ConstValuePtr &defaultValue) const {
  return Path(path).select(*this, defaultValue);
}


ValuePtr Value::select(const string &path) {return Path(path).select(*this);}


ValuePtr Value::select(const string &path, const ValuePtr &defaultValue) {
  return Path(path).select(*this, defaultValue);
}


void Value::appendDict()                {append(createDict());}
void Value::appendList()                {append(createList());}
void Value::appendUndefined()           {append(createUndefined());}
void Value::appendNull()                {append(createNull());}
void Value::appendBoolean(bool value)   {append(createBoolean(value));}
void Value::append(double value)        {append(create(value));}
void Value::append(int64_t value)       {append(create(value));}
void Value::append(uint64_t value)      {append(create(value));}
void Value::append(const string &value) {append(create(value));}


void Value::appendFrom(const Value &value) {
  for (unsigned i = 0; i < value.size(); i++)
    append(value.get(i));
}


void Value::setDict(unsigned i)                  {set(i, createDict());}
void Value::setList(unsigned i)                  {set(i, createList());}
void Value::setUndefined(unsigned i)             {set(i, createUndefined());}
void Value::setNull(unsigned i)                  {set(i, createNull());}
void Value::setBoolean(unsigned i, bool value)   {set(i, createBoolean(value));}
void Value::set(unsigned i, double value)        {set(i, create(value));}
void Value::set(unsigned i, int64_t value)       {set(i, create(value));}
void Value::set(unsigned i, uint64_t value)      {set(i, create(value));}
void Value::set(unsigned i, const string &value) {set(i, create(value));}


unsigned Value::insertDict(const string &key) {
  return insert(key, createDict());
}


unsigned Value::insertList(const string &key) {
  return insert(key, createList());
}


unsigned Value::insertUndefined(const string &key) {
  return insert(key, createUndefined());
}


unsigned Value::insertNull(const string &key) {
  return insert(key, createNull());
}


unsigned Value::insertBoolean(const string &key, bool value) {
  return insert(key, createBoolean(value));
}


unsigned Value::insert(const string &key, double value) {
  return insert(key, create(value));
}


unsigned Value::insert(const string &key, uint64_t value) {
  return insert(key, create(value));
}


unsigned Value::insert(const string &key, int64_t value) {
  return insert(key, create(value));
}


unsigned Value::insert(const string &key, const string &value) {
  return insert(key, create(value));
}


void Value::merge(const Value &value) {
  // Merge lists
  if (isList() && value.isList()) {
    appendFrom(value);
    return;
  }

  if (!isDict() || !value.isDict())
    TYPE_ERROR("Cannot merge JSON nodes of type " << getType() << " and "
               << value.getType());

  // Merge dicts
  for (unsigned i = 0; i < value.size(); i++) {
    const string &key = value.keyAt(i);
    ValuePtr src = value.get(i);

    if (has(key)) {
      ValuePtr dst = get(key);

      if ((src->isDict() && dst->isDict()) ||
          (src->isList() && dst->isList())) {
        dst->merge(*src);
        continue;
      }
    }

    insert(key, src);
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
    if (has(name)) return get(name)->format(type);
    if (type == 'b') return String(false);
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
  if (!depthFirst) visitor(*this, 0, 0);
  visitChildren(visitor, depthFirst);
  if (depthFirst) visitor(*this, 0, 0);
}


void Value::visit(visitor_t visitor, bool depthFirst) {
  if (!depthFirst) visitor(*this, 0, 0);
  visitChildren(visitor, depthFirst);
  if (depthFirst) visitor(*this, 0, 0);
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
