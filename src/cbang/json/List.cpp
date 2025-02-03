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

#include "List.h"
#include "ListIterator.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cctype>

using namespace std;
using namespace cb;
using namespace cb::JSON;


ValuePtr List::copy(bool deep) const {
  ValuePtr c = createList();

  for (auto &v: (ListImpl &)*this)
    c->append(deep ? v->copy(true) : v);

  return c;
}


Iterator List::begin() const {return makeIt(ListImpl::begin());}
Iterator List::end()   const {return makeIt(ListImpl::end());}


Iterator List::find(unsigned i) const {
  return size() <= i ? end() : makeIt(ListImpl::begin() + i);
}


const ValuePtr &List::get(unsigned i) const {
  check(i);
  return ListImpl::operator[](i);
}


void List::append(const ValuePtr &value) {
  if (value->isList() || value->isDict()) simple = false;
  ListImpl::push_back(value);
}


void List::erase(unsigned i) {
  check(i);
  ListImpl::erase(ListImpl::begin() + i);
}


Iterator List::erase(const Iterator &it) {
  return makeIt(ListImpl::erase(ListImpl::begin() + it.index()));
}


void List::write(Sink &sink) const {
  sink.beginList(isSimple());

  for (auto &v: (ListImpl &)*this) {
    if (!v->canWrite(sink)) continue;
    sink.beginAppend();
    v->write(sink);
  }

  sink.endList();
}


void List::set(unsigned i, const ValuePtr &value) {
  if (value.isNull()) THROW("Value cannot be NULL");
  check(i);
  at(i) = value;
}


void List::visitChildren(const_visitor_t visitor, bool depthFirst) const {
  for (auto &_child: (const ListImpl &)*this) {
    auto &child = const_cast<const Value &>(*_child);
    if (depthFirst) child.visitChildren(visitor, depthFirst);
    visitor(child, this);
    if (!depthFirst) child.visitChildren(visitor, depthFirst);
  }
}


void List::visitChildren(visitor_t visitor, bool depthFirst) {
  for (auto &child: (ListImpl &)*this) {
    if (depthFirst) child->visitChildren(visitor, depthFirst);
    visitor(*child, this);
    if (!depthFirst) child->visitChildren(visitor, depthFirst);
  }
}


void List::check(unsigned i) const {
  if (size() <= i) KEY_ERROR("Index " << (int)i << " out of range " << size());
}


Iterator List::makeIt(const ListImpl::const_iterator &it) const {
  return Iterator(new ListIterator(it, ListImpl::begin(), ListImpl::end()));
}
