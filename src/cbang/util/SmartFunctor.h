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

#include <cbang/Catch.h>


namespace cb {
  template <typename T, typename MEMBER = void (T::*)()>
  class SmartFunctor {
    T *obj;
    MEMBER member;
    bool engaged;

  public:
    SmartFunctor(T *obj, MEMBER member, bool engaged = true) :
      obj(obj), member(member), engaged(engaged) {}


    ~SmartFunctor() {
      // Cannot throw an exception during a stack unwind
      if (engaged && obj) TRY_CATCH_ERROR((*obj.*member)());
    }

    bool isEngaged() const {return engaged;}
    void setEngaged(bool engaged) {this->engaged = engaged;}
  };


  template <typename T, typename MEMBER = void (T::*)() const>
  class SmartConstFunctor : public SmartFunctor<const T, MEMBER> {
  public:
    SmartConstFunctor(const T *obj, MEMBER member, bool engaged = true) :
      SmartFunctor<const T, MEMBER>(obj, member, engaged) {}
  };
}
