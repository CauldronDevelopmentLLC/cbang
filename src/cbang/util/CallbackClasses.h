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

#include "MemberFunctor.h"
#include "Function.h"


#define CBANG_CALLBACK_CLASSES(CLASS, RETURN, CALLBACK, CONST, ...)  \
  CBANG_FUNCTOR_(CBANG_CAT(CLASS, Functor), CLASS, RETURN, CALLBACK, CONST, \
                 __VA_ARGS__);                                          \
  CBANG_MEMBER_FUNCTOR_(CBANG_CAT(CLASS, MemberFunctor), CLASS, RETURN, \
                        CALLBACK, CONST, __VA_ARGS__);                  \
  CBANG_FUNCTION_(CBANG_CAT(CLASS, Function), CLASS, RETURN, CALLBACK,  \
                  CONST, __VA_ARGS__)

#define CBANG_CALLBACK_CLASSES_WITH_BASE(CLASS, RETURN, CALLBACK, CONST, ...) \
  CBANG_FUNCTOR_BASE(CLASS, RETURN, CALLBACK, CONST, __VA_ARGS__);      \
  CBANG_CALLBACK_CLASSES(CLASS, RETURN, CALLBACK, CONST, __VA_ARGS__)


namespace cb {
  namespace __callback_classes_test__ {
    CBANG_CALLBACK_CLASSES_WITH_BASE(_0, void, foo, const);
    CBANG_CALLBACK_CLASSES_WITH_BASE(_1, void, foo, const, int);
    CBANG_CALLBACK_CLASSES_WITH_BASE(_2, void, foo, const, int, int);
  }
}
