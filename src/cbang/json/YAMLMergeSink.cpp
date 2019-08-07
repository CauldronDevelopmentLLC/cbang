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

#include "YAMLMergeSink.h"

using namespace std;
using namespace cb::JSON;


bool YAMLMergeSink::inRoot() const {
  return !getDepth() || (getDepth() == 1 && list);
}


bool YAMLMergeSink::inRootDict() const {
  return getDepth() == 1 || (list && getDepth() == 2);
}


void YAMLMergeSink::writeNull() {
  assertNotInRoot();
  ProxySink::writeNull();
}


void YAMLMergeSink::writeBoolean(bool value) {
  assertNotInRoot();
  ProxySink::writeBoolean(value);
}


void YAMLMergeSink::write(double value) {
  assertNotInRoot();
  ProxySink::write(value);
}


void YAMLMergeSink::write(int8_t value) {
  assertNotInRoot();
  ProxySink::write(value);
}



void YAMLMergeSink::write(uint8_t value) {
  assertNotInRoot();
  ProxySink::write(value);
}


void YAMLMergeSink::write(int16_t value) {
  assertNotInRoot();
  ProxySink::write(value);
}


void YAMLMergeSink::write(uint16_t value) {
  assertNotInRoot();
  ProxySink::write(value);
}


void YAMLMergeSink::write(int32_t value) {
  assertNotInRoot();
  ProxySink::write(value);
}


void YAMLMergeSink::write(uint32_t value) {
  assertNotInRoot();
  ProxySink::write(value);
}


void YAMLMergeSink::write(int64_t value) {
  assertNotInRoot();
  ProxySink::write(value);
}


void YAMLMergeSink::write(uint64_t value) {
  assertNotInRoot();
  ProxySink::write(value);
}


void YAMLMergeSink::write(const string &value) {
  assertNotInRoot();
  ProxySink::write(value);
}


void YAMLMergeSink::beginList(bool simple) {
  if (getDepth()) target->beginList(simple);
  else list = true;
  NullSink::beginList(simple);
}


void YAMLMergeSink::beginAppend() {
  if (getDepth() != 1) target->beginAppend();
  NullSink::beginAppend();
}


void YAMLMergeSink::endList() {
  NullSink::endList();
  if (getDepth()) target->endList();
  else list = false;
}


void YAMLMergeSink::beginDict(bool simple) {
  if (!inRoot()) target->beginDict(simple);
  NullSink::beginDict(simple);
}


void YAMLMergeSink::beginInsert(const string &key) {
  // Ignore items already in the target
  if (inRootDict()) setTarget(has(key) ? 0 : target);
  ProxySink::beginInsert(key);
}


void YAMLMergeSink::endDict() {
  NullSink::endDict();
  if (!inRoot()) target->endDict();
}


void YAMLMergeSink::assertNotInRoot() const {
  if (inRoot()) THROW("Unexpected scalar write in YAML merge");
}
