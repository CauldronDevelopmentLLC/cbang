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

#include "Factory.h"

#include "Dict.h"
#include "List.h"
#include "Undefined.h"
#include "Null.h"
#include "Number.h"
#include "True.h"
#include "False.h"
#include "String.h"

using namespace std;
using namespace cb::JSON;


ValuePtr Factory::createDict() const {return new Dict;}
ValuePtr Factory::createList() const {return new List;}
ValuePtr Factory::createUndefined() const {return Undefined::instancePtr();}
ValuePtr Factory::createNull() const {return Null::instancePtr();}


ValuePtr Factory::createBoolean(bool value) const {
  return value ? True::instancePtr() : False::instancePtr();
}


ValuePtr Factory::create(double value) const {return new Number(value);}
ValuePtr Factory::create(float value) const {return create((double)value);}
ValuePtr Factory::create(int8_t value) const {return create((int64_t)value);}
ValuePtr Factory::create(uint8_t value) const {return create((uint64_t)value);}
ValuePtr Factory::create(int16_t value) const {return create((int64_t)value);}
ValuePtr Factory::create(uint16_t value) const {return create((uint64_t)value);}
ValuePtr Factory::create(int32_t value) const {return create((int64_t)value);}
ValuePtr Factory::create(uint32_t value) const {return create((uint64_t)value);}
ValuePtr Factory::create(int64_t value) const {return new S64(value);}
ValuePtr Factory::create(uint64_t value) const {return new U64(value);}
ValuePtr Factory::create(const string &value) const {return new String(value);}
