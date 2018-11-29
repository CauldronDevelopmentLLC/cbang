/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2017, Cauldron Development LLC
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

#include <sstream>
#include <limits>

using namespace std;
using namespace cb::JSON;


void Value::appendDict() {append(createDict());}
void Value::appendList() {append(createList());}
void Value::appendUndefined() {append(createUndefined());}
void Value::appendNull() {append(createNull());}
void Value::appendBoolean(bool value) {append(createBoolean(value));}
void Value::append(double value) {append(create(value));}
void Value::append(int64_t value) {append(create(value));}
void Value::append(uint64_t value) {append(create(value));}
void Value::append(const string &value) {append(create(value));}

void Value::setDict(unsigned i) {set(i, createDict());}
void Value::setList(unsigned i) {set(i, createList());}
void Value::setUndefined(unsigned i) {set(i, createUndefined());}
void Value::setNull(unsigned i) {set(i, createNull());}
void Value::setBoolean(unsigned i, bool value) {set(i, createBoolean(value));}
void Value::set(unsigned i, double value) {set(i, create(value));}
void Value::set(unsigned i, int64_t value) {set(i, create(value));}
void Value::set(unsigned i, uint64_t value) {set(i, create(value));}
void Value::set(unsigned i, const string &value) {set(i, create(value));}

void Value::insertDict(const string &key) {insert(key, createDict());}
void Value::insertList(const string &key) {insert(key, createList());}
void Value::insertUndefined(const string &key) {insert(key, createUndefined());}
void Value::insertNull(const string &key) {insert(key, createNull());}


void Value::insertBoolean(const string &key, bool value) {
  insert(key, createBoolean(value));
}


void Value::insert(const string &key, double value) {
  insert(key, create(value));
}


void Value::insert(const string &key, uint64_t value) {
  insert(key, create(value));
}


void Value::insert(const string &key, int64_t value) {
  insert(key, create(value));
}


void Value::insert(const string &key, const string &value) {
  insert(key, create(value));
}


void Value::merge(const Value &value) {
  // Merge lists
  if (isList() && value.isList()) {
    for (unsigned i = 0; i < value.size(); i++)
      append(value.get(i));

    return;
  }

  if (!isDict() || !value.isDict())
    THROWS("Cannot merge JSON nodes of type " << getType() << " and "
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


string::const_iterator
Value::format(string &result, const string &defaultValue,
              string::const_iterator start, string::const_iterator end) const {
  string::const_iterator it = start + 1;

  string name;
  while (it != end && *it != ')') name.push_back(*it++);

  if (it == end || ++it == end) return start;

  if (!has(name)) result.append(defaultValue);
  else switch (*it) {
    case 'b': result.append(String(getBoolean(name))); break;
    case 'f': result.append(String(getNumber(name))); break;
    case 'i': result.append(String(getS32(name))); break;
    case 'u': result.append(String(getU32(name))); break;
    case 's':
      result.append("\"" + String::escapeC(getString(name)) + "\"");
      break;
    default: return start;
    }

  return ++it;
}


string Value::format(const string &s, const string &defaultValue) const {
  string result;
  result.reserve(s.length());

  bool escape = false;

  for (string::const_iterator it = s.begin(); it != s.end(); it++) {
    if (escape) {
      escape  = false;
      if (*it == '(') {
        it = format(result, defaultValue, it, s.end());
        if (it == s.end()) break;
      }

    } else if (*it == '%') {
      escape = true;
      continue;
    }

    result.push_back(*it);
  }

  return result;
}


string Value::toString(unsigned indent, bool compact) const {
  ostringstream str;
  Writer writer(str, indent, compact);
  write(writer);
  str << flush;
  return str.str();
}


string Value::asString() const {return isString() ? getString() : toString();}
