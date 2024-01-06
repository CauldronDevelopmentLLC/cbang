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

#include "List.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cctype>

using namespace std;
using namespace cb;
using namespace cb::JSON;


ValuePtr List::copy(bool deep) const {
  ValuePtr c = createList();

  for (unsigned i = 0; i < size(); i++)
    c->append(deep ? at(i)->copy(true) : at(i));

  return c;
}


const ValuePtr &List::get(unsigned i) const {
  check(i);
  return Super_T::operator[](i);
}


void List::append(const ValuePtr &value) {
  if (value->isList() || value->isDict()) simple = false;
  push_back(value);
}


void List::erase(unsigned i) {
  check(i);
  Super_T::erase(Super_T::begin() + i);
}


void List::write(Sink &sink) const {
  sink.beginList(isSimple());

  for (auto it = Super_T::begin(); it != Super_T::end(); it++) {
    if (!(*it)->canWrite(sink)) continue;
    sink.beginAppend();
    (*it)->write(sink);
  }

  sink.endList();
}


void List::set(unsigned i, const ValuePtr &value) {
  if (value.isNull()) THROW("Value cannot be NULL");
  check(i);
  at(i) = value;
}


void List::visitChildren(const_visitor_t visitor, bool depthFirst) const {
  for (unsigned i = 0; i < size(); i++) {
    const Value &child = *get(i);

    if (depthFirst) child.visitChildren(visitor, depthFirst);
    visitor(child, this, i);
    if (!depthFirst) child.visitChildren(visitor, depthFirst);
  }
}


void List::visitChildren(visitor_t visitor, bool depthFirst) {
  for (unsigned i = 0; i < size(); i++) {
    Value &child = *get(i);

    if (depthFirst) child.visitChildren(visitor, depthFirst);
    visitor(child, this, i);
    if (!depthFirst) child.visitChildren(visitor, depthFirst);
  }
}


void List::check(unsigned i) const {
  if (size() <= i) KEY_ERROR("Index " << i << " out of range " << size());
}
