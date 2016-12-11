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

#ifndef CB_JSON_UNDEFINED_H
#define CB_JSON_UNDEFINED_H

#include "Value.h"


namespace cb {
  namespace JSON {
    class Undefined : public Value {
      static Undefined undefined;

      Undefined() {}
      ~Undefined() {}

    public:
      inline static Undefined &instance() {return undefined;}
      inline static ValuePtr instancePtr() {return ValuePtr::Phony(&undefined);}

      // From Value
      ValueType getType() const {return JSON_UNDEFINED;}
      ValuePtr copy(bool deep = false) const {return instancePtr();}
      bool canWrite(Sink &sink) const {return false;}
      void write(Sink &sink) const {}
    };
  }
}

#endif // CB_JSON_UNDEFINED_H
