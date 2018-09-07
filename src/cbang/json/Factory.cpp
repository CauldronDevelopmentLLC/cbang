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

#include "Factory.h"

#include "Dict.h"
#include "List.h"
#include "Undefined.h"
#include "Null.h"
#include "Number.h"
#include "Boolean.h"
#include "String.h"

using namespace std;
using namespace cb::JSON;


ValuePtr Factory::createDict() {return new Dict;}
ValuePtr Factory::createList() {return new List;}
ValuePtr Factory::createUndefined() {return Undefined::instancePtr();}
ValuePtr Factory::createNull() {return Null::instancePtr();}
ValuePtr Factory::createBoolean(bool value) {return new Boolean(value);}
ValuePtr Factory::create(double value) {return new Number(value);}
ValuePtr Factory::create(float value) {return create((double)value);}
ValuePtr Factory::create(int8_t value) {return create((int64_t)value);}
ValuePtr Factory::create(uint8_t value) {return create((uint64_t)value);}
ValuePtr Factory::create(int16_t value) {return create((int64_t)value);}
ValuePtr Factory::create(uint16_t value) {return create((uint64_t)value);}
ValuePtr Factory::create(int32_t value) {return create((int64_t)value);}
ValuePtr Factory::create(uint32_t value) {return create((uint64_t)value);}
ValuePtr Factory::create(int64_t value) {return new S64(value);}
ValuePtr Factory::create(uint64_t value) {return new U64(value);}
ValuePtr Factory::create(const string &value) {return new String(value);}
