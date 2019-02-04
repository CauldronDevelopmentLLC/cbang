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

#pragma once

#include <cbang/SmartPointer.h>
#include <cbang/StdTypes.h>


namespace cb {
  namespace JSON {
    class Value;

    typedef cb::SmartPointer<Value> ValuePtr;

    class Factory {
    public:
      static ValuePtr createDict();
      static ValuePtr createList();
      static ValuePtr createUndefined();
      static ValuePtr createNull();
      static ValuePtr createBoolean(bool value);
      static ValuePtr create(double value);
      static ValuePtr create(float value);
      static ValuePtr create(int8_t value);
      static ValuePtr create(uint8_t value);
      static ValuePtr create(int16_t value);
      static ValuePtr create(uint16_t value);
      static ValuePtr create(int32_t value);
      static ValuePtr create(uint32_t value);
      static ValuePtr create(int64_t value);
      static ValuePtr create(uint64_t value);
      static ValuePtr create(const std::string &value);
    };
  }
}
