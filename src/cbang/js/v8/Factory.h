/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2024, Cauldron Development  Oy
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

#pragma once

#include "Value.h"

#include <cbang/js/Factory.h>


namespace cb {
  namespace gv8 {
    class Factory : public js::Factory {
      Value trueValue;
      Value falseValue;
      Value undefinedValue;
      Value nullValue;

    public:
      Factory();

      // From js::Factory
      SmartPointer<js::Value> create(const std::string &value) override;
      SmartPointer<js::Value> create(double value) override;
      SmartPointer<js::Value> create(int32_t value) override;
      SmartPointer<js::Value> create(const js::Function &func) override;
      SmartPointer<js::Value> createArray(unsigned size) override;
      SmartPointer<js::Value> createObject() override;
      SmartPointer<js::Value> createBoolean(bool value) override;
      SmartPointer<js::Value> createUndefined() override;
      SmartPointer<js::Value> createNull() override;
    };
  }
}
