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

#include "NullSink.h"

#include <cbang/Errors.h>


using namespace cb::JSON;


void NullSink::close() {
  if (!stack.empty()) THROW("Writer closed with open " << stack.back());
}


void NullSink::reset() {
  stack.clear();
  keyStack.clear();
  canWrite = true;
}


bool NullSink::inList() const {
  return !stack.empty() && stack.back() == ValueType::JSON_LIST;
}


void NullSink::beginList(bool simple) {
  assertCanWrite();
  stack.push_back(ValueType::JSON_LIST);
  canWrite = false;
}


void NullSink::beginAppend() {
  assertWriteNotPending();
  if (!inList()) TYPE_ERROR("Not a List");
  canWrite = true;
}


void NullSink::endList() {
  assertWriteNotPending();
  if (!inList()) TYPE_ERROR("Not a List");

  stack.pop_back();
}


bool NullSink::inDict() const {
  return !stack.empty() && stack.back() == ValueType::JSON_DICT;
}


void NullSink::beginDict(bool simple) {
  assertCanWrite();
  stack.push_back(ValueType::JSON_DICT);
  keyStack.push_back(keys_t());
  canWrite = false;
}


bool NullSink::has(const std::string &key) const {
  if (!inDict()) TYPE_ERROR("Not a Dict");
  return keyStack.back().find(key) != keyStack.back().end();
}


void NullSink::beginInsert(const std::string &key) {
  assertWriteNotPending();
  if (has(key) && !allowDuplicates)
    KEY_ERROR("Key '" << key << "' already written to output");
  keyStack.back().insert(key);
  canWrite = true;
}


void NullSink::endDict() {
  assertWriteNotPending();
  if (!inDict()) TYPE_ERROR("Not a Dict");

  stack.pop_back();
  keyStack.pop_back();
}


void NullSink::assertCanWrite() {
  if (!canWrite) THROW("Not ready for write");
  canWrite = false;
}


void NullSink::assertWriteNotPending() {
  if (canWrite) THROW("Expected write");
}
