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

#pragma once

#include "Tokenizer.h"

#include <cbang/SmartPointer.h>

#include <ostream>
#include <vector>

namespace cb {
  template <typename ENUM_T>
  class ASTNode {
    ENUM_T type;
    LocationRange location;

  public:
    ASTNode(ENUM_T type) : type(type) {}
    virtual ~ASTNode() {} // Compiler needs this

    ENUM_T getType() const {return type;}
    void setType(ENUM_T type) {this->type = type;}

    LocationRange &getLocation() const {return location;}
    LocationRange &getLocation() {return location;}
  };
}
