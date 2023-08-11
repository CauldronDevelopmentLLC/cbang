/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include "TeeSink.h"

using namespace std;
using namespace cb::JSON;


void TeeSink::writeNull() {
  left->writeNull();
  right->writeNull();
}


void TeeSink::writeBoolean(bool value) {
  left->writeBoolean(value);
  right->writeBoolean(value);
}


void TeeSink::write(double value) {
  left->write(value);
  right->write(value);
}


void TeeSink::write(int8_t value) {
  left->write(value);
  right->write(value);
}



void TeeSink::write(uint8_t value) {
  left->write(value);
  right->write(value);
}


void TeeSink::write(int16_t value) {
  left->write(value);
  right->write(value);
}


void TeeSink::write(uint16_t value) {
  left->write(value);
  right->write(value);
}


void TeeSink::write(int32_t value) {
  left->write(value);
  right->write(value);
}


void TeeSink::write(uint32_t value) {
  left->write(value);
  right->write(value);
}


void TeeSink::write(int64_t value) {
  left->write(value);
  right->write(value);
}


void TeeSink::write(uint64_t value) {
  left->write(value);
  right->write(value);
}


void TeeSink::write(const string &value) {
  left->write(value);
  right->write(value);
}


void TeeSink::beginList(bool simple) {
  left->beginList(simple);
  right->beginList(simple);
}


void TeeSink::beginAppend() {
  left->beginAppend();
  right->beginAppend();
}


void TeeSink::endList() {
  left->endList();
  right->endList();
}


void TeeSink::beginDict(bool simple) {
  left->beginDict(simple);
  right->beginDict(simple);
}


bool TeeSink::has(const string &key) const {
  return left->has(key) || right->has(key);
}


void TeeSink::beginInsert(const string &key) {
  left->beginInsert(key);
  right->beginInsert(key);
}


void TeeSink::endDict() {
  left->endDict();
  right->endDict();
}
