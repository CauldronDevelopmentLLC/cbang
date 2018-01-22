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


void Value::appendUndefined() {append(createUndefined());}
void Value::appendNull() {append(createNull());}
void Value::appendBoolean(bool value) {append(createBoolean(value));}
void Value::append(double value) {append(create(value));}
void Value::append(int64_t value) {append(create(value));}
void Value::append(uint64_t value) {append(create(value));}
void Value::append(const string &value) {append(create(value));}

void Value::setUndefined(unsigned i) {set(i, createUndefined());}
void Value::setNull(unsigned i) {set(i, createNull());}
void Value::setBoolean(unsigned i, bool value) {set(i, createBoolean(value));}
void Value::set(unsigned i, double value) {set(i, create(value));}
void Value::set(unsigned i, int64_t value) {set(i, create(value));}
void Value::set(unsigned i, uint64_t value) {set(i, create(value));}
void Value::set(unsigned i, const string &value) {set(i, create(value));}


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


string Value::toString(unsigned indent, bool compact) const {
  ostringstream str;
  Writer writer(str, indent, compact);
  write(writer);
  str << flush;
  return str.str();
}
