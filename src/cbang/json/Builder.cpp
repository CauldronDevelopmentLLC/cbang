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

#include "Builder.h"
#include "JSON.h"

using namespace std;
using namespace cb;
using namespace cb::JSON;


Builder::Builder(const ValuePtr &root) : root(root) {}


ValuePtr Builder::build(function<void (Sink &sink)> cb) {
  Builder builder;
  cb(builder);
  return builder.getRoot();
}


void Builder::close() {
  if (!stack.empty())
    THROW("JSON::Builder closed with open " << stack.back()->getType());
}


void Builder::reset() {
  stack.clear();
  nextKey.clear();
  appendNext = insertNext = false;
  root.release();
}


void Builder::writeNull()                {add(createNull());}
void Builder::writeBoolean(bool value)   {add(createBoolean(value));}
void Builder::write(double value)        {add(create(value));}
void Builder::write(uint64_t value)      {add(create(value));}
void Builder::write(int64_t value)       {add(create(value));}
void Builder::write(const string &value) {add(create(value));}


bool Builder::inList() const {return !stack.empty() && stack.back()->isList();}
void Builder::beginList(bool simple) {add(createList());}


void Builder::beginAppend() {
  if (!inList()) TYPE_ERROR("Not a List");
  assertNotPending();
  appendNext = true;
}


void Builder::endList() {
  assertNotPending();
  if (inList()) stack.pop_back();
  else TYPE_ERROR("Not a List");
}


bool Builder::inDict() const {return !stack.empty() && stack.back()->isDict();}
void Builder::beginDict(bool simple) {add(createDict());}


bool Builder::has(const string &key) const {
  if (!inDict()) TYPE_ERROR("Not a Dict");
  return stack.back()->has(key);
}


void Builder::beginInsert(const string &key) {
  if (!inDict()) TYPE_ERROR("Not a Dict");
  assertNotPending();
  nextKey    = key;
  insertNext = true;
}


void Builder::endDict() {
  assertNotPending();
  if (inDict()) stack.pop_back();
  else TYPE_ERROR("Not a Dict");
}


void Builder::add(const ValuePtr &value) {
  if (appendNext) {
    appendNext = false;
    stack.back()->append(value);

  } else if (insertNext) {
    stack.back()->insert(nextKey, value);
    nextKey.clear();
    insertNext = false;

  } else if (root.isNull()) root = value;
  else THROW("Cannot add " << value->getType());

  if (value->isList() || value->isDict()) stack.push_back(value);
}


void Builder::assertNotPending() {
  if (appendNext) THROW("Already called beginAppend()");
  if (insertNext) THROW("Already called beginAssert()");
}
