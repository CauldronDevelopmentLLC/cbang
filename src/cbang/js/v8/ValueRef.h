/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
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


namespace cb {
  namespace gv8 {
    // A Value which survives the HandleScope it was created in.  The base
    // class' Local is cleared because it becomes invalid once that scope
    // exits; getV8Value() returns a new Local, from the persistent handle, in
    // the caller's current HandleScope.
    class ValueRef : public Value {
      v8::Global<v8::Value> ref; // Reset on destruction

    public:
      ValueRef(const Value &value = Value()) :
        ref(getIso(), value.getV8Value()) {this->value.Clear();}

      ValueRef(const ValueRef &) = delete;
      ValueRef &operator=(const ValueRef &) = delete;

      // From Value
      v8::Local<v8::Value> getV8Value() const override
      {return ref.Get(getIso());}
    };
  }
}
