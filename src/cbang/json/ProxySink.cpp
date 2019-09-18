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

#include "ProxySink.h"

using namespace std;
using namespace cb::JSON;


void ProxySink::writeNull() {
  NullSink::writeNull();
  if (target.isSet()) target->writeNull();
}


void ProxySink::writeBoolean(bool value) {
  NullSink::writeBoolean(value);
  if (target.isSet()) target->writeBoolean(value);
}


void ProxySink::write(double value) {
  NullSink::write(value);
  if (target.isSet()) target->write(value);
}


void ProxySink::write(int8_t value) {
  NullSink::write(value);
  if (target.isSet()) target->write(value);
}



void ProxySink::write(uint8_t value) {
  NullSink::write(value);
  if (target.isSet()) target->write(value);
}


void ProxySink::write(int16_t value) {
  NullSink::write(value);
  if (target.isSet()) target->write(value);
}


void ProxySink::write(uint16_t value) {
  NullSink::write(value);
  if (target.isSet()) target->write(value);
}


void ProxySink::write(int32_t value) {
  NullSink::write(value);
  if (target.isSet()) target->write(value);
}


void ProxySink::write(uint32_t value) {
  NullSink::write(value);
  if (target.isSet()) target->write(value);
}


void ProxySink::write(int64_t value) {
  NullSink::write(value);
  if (target.isSet()) target->write(value);
}


void ProxySink::write(uint64_t value) {
  NullSink::write(value);
  if (target.isSet()) target->write(value);
}


void ProxySink::write(const string &value) {
  NullSink::write(value);
  if (target.isSet()) target->write(value);
}


void ProxySink::beginList(bool simple) {
  NullSink::beginList(simple);
  if (target.isSet()) target->beginList(simple);
}


void ProxySink::beginAppend() {
  NullSink::beginAppend();
  if (target.isSet()) target->beginAppend();
}


void ProxySink::endList() {
  NullSink::endList();
  if (target.isSet()) target->endList();
}


void ProxySink::beginDict(bool simple) {
  NullSink::beginDict(simple);
  if (target.isSet()) target->beginDict(simple);
}


bool ProxySink::has(const string &key) const {
  return NullSink::has(key) || (target.isSet() && target->has(key));
}


void ProxySink::beginInsert(const string &key) {
  NullSink::beginInsert(key);
  if (target.isSet()) target->beginInsert(key);
}


void ProxySink::endDict() {
  NullSink::endDict();
  if (target.isSet()) target->endDict();
}
