/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2015, Cauldron Development LLC
                 Copyright (c) 2003-2015, Stanford University
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

#include "Factory.h"
#include "Value.h"

using namespace cb::gv8;
using namespace cb;
using namespace std;


Factory::Factory() :
  trueValue(true), falseValue(false), undefinedValue(),
  nullValue(Value::createNull()) {}


SmartPointer<js::Value> Factory::create(const string &value) {
  return new Value(value);
}


SmartPointer<js::Value> Factory::create(double value) {
  return new Value(value);
}


SmartPointer<js::Value> Factory::create(int32_t value) {
  return new Value(value);
}


SmartPointer<js::Value> Factory::create(const js::Function &func) {
  return new Value(func);
}


SmartPointer<js::Value> Factory::createArray(unsigned size) {
  return new Value(Value::createArray(size));
}


SmartPointer<js::Value> Factory::createObject() {
  return new Value(Value::createObject());
}


SmartPointer<js::Value> Factory::createBoolean(bool value) {
  return SmartPointer<js::Value>::Phony(&(value ? trueValue : falseValue));
}


SmartPointer<js::Value> Factory::createUndefined() {
  return SmartPointer<js::Value>::Phony(&undefinedValue);
}


SmartPointer<js::Value> Factory::createNull() {
  return SmartPointer<js::Value>::Phony(&nullValue);
}
