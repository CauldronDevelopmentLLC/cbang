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


#define CBANG_CBC(CLASS, RETURN, CALLBACK, CONST, ARGS, PARAMS)         \
  CBANG_FUNC(CBANG_CONCAT(CLASS, Functor), CLASS, RETURN, CALLBACK, CONST, \
             ARGS, PARAMS);                                             \
  CBANG_MF(CBANG_CONCAT(CLASS, MemberFunctor), CLASS, RETURN, CALLBACK, CONST, \
           ARGS, PARAMS);                                               \
  CBANG_FUN(CBANG_CONCAT(CLASS, Function), CLASS, RETURN, CALLBACK, CONST, \
            ARGS, PARAMS)


#define CBANG_CALLBACK_CLASSES(CLASS, RETURN, CALLBACK, CONST)  \
  CBANG_CBC(CLASS, RETURN, CALLBACK, CONST, , )

#define CBANG_CALLBACK_CLASSES1(CLASS, RETURN, CALLBACK, CONST, ARG1) \
  CBANG_CBC(CLASS, RETURN, CALLBACK, CONST, CBANG_ARGS1(ARG1), CBANG_PARAMS1())

#define CBANG_CALLBACK_CLASSES2(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2) \
  CBANG_CBC(CLASS, RETURN, CALLBACK, CONST, CBANG_ARGS2(ARG1, ARG2), \
            CBANG_PARAMS2())

#define CBANG_CALLBACK_CLASSES3(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2, \
                                ARG3)                                   \
  CBANG_CBC(CLASS, RETURN, CALLBACK, CONST, CBANG_ARGS3(ARG1, ARG2, ARG3), \
            CBANG_PARAMS3())

#define CBANG_CALLBACK_CLASSES4(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2, \
                                ARG3, ARG4)                             \
  CBANG_CBC(CLASS, RETURN, CALLBACK, CONST,                             \
            CBANG_ARGS4(ARG1, ARG2, ARG3, ARG4), CBANG_PARAMS4())

#define CBANG_CALLBACK_CLASSES5(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2, \
                                ARG3, ARG4, ARG5)                       \
  CBANG_CBC(CLASS, RETURN, CALLBACK, CONST,                             \
            CBANG_ARGS5(ARG1, ARG2, ARG3, ARG4, ARG5), CBANG_PARAMS5())


// With Base
#define CBANG_CALLBACK_CLASSES_WITH_BASE(CLASS, RETURN, CALLBACK, CONST) \
  CBANG_CALLBACK_CLASSES(CLASS, RETURN, CALLBACK, CONST);               \
  CBANG_FUNCTOR_BASE(CLASS, RETURN, CALLBACK, CONST)

#define CBANG_CALLBACK_CLASSES_WITH_BASE1(CLASS, RETURN, CALLBACK, CONST, \
                                          ARG1)                         \
  CBANG_CALLBACK_CLASSES1(CLASS, RETURN, CALLBACK, CONST, ARG1);        \
  CBANG_FUNCTOR_BASE1(CLASS, RETURN, CALLBACK, CONST, ARG1)

#define CBANG_CALLBACK_CLASSES_WITH_BASE2(CLASS, RETURN, CALLBACK, CONST, \
                                          ARG1, ARG2)                   \
  CBANG_CALLBACK_CLASSES2(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2);  \
  CBANG_FUNCTOR_BASE2(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2)

#define CBANG_CALLBACK_CLASSES_WITH_BASE3(CLASS, RETURN, CALLBACK, CONST, \
                                          ARG1, ARG2, ARG3)             \
  CBANG_CALLBACK_CLASSES3(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2, ARG3); \
  CBANG_FUNCTOR_BASE3(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2, ARG3)

#define CBANG_CALLBACK_CLASSES_WITH_BASE4(CLASS, RETURN, CALLBACK, CONST, \
                                          ARG1, ARG2, ARG3, ARG4)       \
  CBANG_CALLBACK_CLASSES4(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2, ARG3, \
                          ARG4);                                        \
  CBANG_FUNCTOR_BASE4(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2, ARG3, ARG4)

#define CBANG_CALLBACK_CLASSES_WITH_BASE5(CLASS, RETURN, CALLBACK, CONST, \
                                          ARG1, ARG2,ARG3, ARG4, ARG5)  \
  CBANG_CALLBACK_CLASSES5(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2, ARG3, \
                          ARG4, ARG5);                                  \
  CBANG_FUNCTOR_BASE5(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2, ARG3, ARG4, \
                      ARG5)


namespace cb {
  namespace __callback_classes_test__ {
    CBANG_CALLBACK_CLASSES_WITH_BASE(_0, void, foo, const);
    CBANG_CALLBACK_CLASSES_WITH_BASE1(_1, void, foo, const, int);
    CBANG_CALLBACK_CLASSES_WITH_BASE2(_2, void, foo, const, int, int);
    CBANG_CALLBACK_CLASSES_WITH_BASE3(_3, void, foo, const, int, int, int);
    CBANG_CALLBACK_CLASSES_WITH_BASE4(_4, void, foo, const, int, int, int, int);
    CBANG_CALLBACK_CLASSES_WITH_BASE5(_5, void, foo, const, int, int, int, \
                                      int, int);
  }
}
