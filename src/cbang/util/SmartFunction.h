/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
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

#include <cbang/Catch.h>

#include <functional>


namespace cb {
  class SmartFunction {
    typedef std::function<void ()> callback_t;
    callback_t cb;
    bool engaged;

  public:
    explicit SmartFunction(callback_t cb, bool engaged = true) :
      cb(cb), engaged(engaged) {}


    ~SmartFunction() {
      // Cannot throw an exception during a stack unwind
      if (engaged && cb) TRY_CATCH_ERROR(cb());
    }

    bool isEngaged() const {return engaged;}
    void setEngaged(bool engaged) {this->engaged = engaged;}
  };
}
