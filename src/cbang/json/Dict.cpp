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

#include "Dict.h"
#include "DictIterator.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cctype>

using namespace std;
using namespace cb::JSON;


ValuePtr Dict::copy(bool deep) const {
  ValuePtr c = createDict();

  for (auto e: entries())
    c->insert(e.key(), deep ? e.value()->copy(true) : e.value());

  return c;
}


Iterator Dict::begin() const {return makeIt(DictImpl::begin());}
Iterator Dict::end()   const {return makeIt(DictImpl::end());}


Iterator Dict::find(const string &key) const {
  return makeIt(DictImpl::find(key));
}


const ValuePtr &Dict::get(const string &key) const {
  auto it = DictImpl::find(key);
  if (it == DictImpl::end()) CBANG_KEY_ERROR("Key '" << key << "' not found");
  return it.value();
}


Iterator Dict::insert(const string &key, const ValuePtr &value) {
  if (value->isList() || value->isDict()) simple = false;
  return makeIt(DictImpl::insert(key, value));
}


Iterator Dict::erase(const Iterator &it) {
  return makeIt(DictImpl::erase(it.key()));
}


void Dict::write(Sink &sink) const {
  sink.beginDict(isSimple());

  for (auto it: (DictImpl &)*this) {
    if (!it.value()->canWrite(sink)) continue;
    sink.beginInsert(it.key());
    it.value()->write(sink);
  }

  sink.endDict();
}


void Dict::visitChildren(const_visitor_t visitor, bool depthFirst) const {
  for (auto &_child: (const Value &)*this) {
    auto &child = const_cast<const Value &>(*_child);
    if (depthFirst) child.visitChildren(visitor, depthFirst);
    visitor(child, this);
    if (!depthFirst) child.visitChildren(visitor, depthFirst);
  }
}


void Dict::visitChildren(visitor_t visitor, bool depthFirst) {
  for (auto &child: (Value &)*this) {
    if (depthFirst) child->visitChildren(visitor, depthFirst);
    visitor(*child, this);
    if (!depthFirst) child->visitChildren(visitor, depthFirst);
  }
}


Iterator Dict::makeIt(const DictImpl::const_iterator &it) const {
  return Iterator(new DictIterator(it, DictImpl::end()));
}
