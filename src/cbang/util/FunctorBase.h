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

#include <cbang/macro/MacroUtils.h>


#define CBANG_FUNC_BASE(CLASS, RETURN, CALLBACK, CONST, ARGS)           \
  class CLASS {                                                         \
  public:                                                               \
    virtual ~CLASS() {}                                                 \
    virtual RETURN CALLBACK(ARGS) CONST = 0;                            \
  }


#define CBANG_ARGS1(ARG1) ARG1 arg1
#define CBANG_ARGS2(ARG1, ARG2) \
  CBANG_ARGS1(ARG1) CBANG_DEFER(CBANG_COMMA)() ARG2 arg2
#define CBANG_ARGS3(ARG1, ARG2, ARG3)                           \
  CBANG_ARGS2(ARG1, ARG2) CBANG_DEFER(CBANG_COMMA)() ARG3 arg3
#define CBANG_ARGS4(ARG1, ARG2, ARG3, ARG4)                     \
  CBANG_ARGS3(ARG1, ARG2, ARG3) CBANG_DEFER(CBANG_COMMA)() ARG4 arg4
#define CBANG_ARGS5(ARG1, ARG2, ARG3, ARG4, ARG5)                       \
  CBANG_ARGS4(ARG1, ARG2, ARG3, ARG4) CBANG_DEFER(CBANG_COMMA)() ARG5 arg5


#define CBANG_FUNCTOR_BASE(CLASS, RETURN, CALLBACK, CONST)      \
  CBANG_FUNC_BASE(CLASS, RETURN, CALLBACK, CONST,)

#define CBANG_FUNCTOR_BASE1(CLASS, RETURN, CALLBACK, CONST, ARG1)       \
  CBANG_FUNC_BASE(CLASS, RETURN, CALLBACK, CONST, CBANG_ARGS1(ARG1))

#define CBANG_FUNCTOR_BASE2(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2) \
  CBANG_FUNC_BASE(CLASS, RETURN, CALLBACK, CONST, CBANG_ARGS2(ARG1, ARG2))

#define CBANG_FUNCTOR_BASE3(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2, ARG3) \
  CBANG_FUNC_BASE(CLASS, RETURN, CALLBACK, CONST, CBANG_ARGS3(ARG1, ARG2, ARG3))

#define CBANG_FUNCTOR_BASE4(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2, ARG3, \
                            ARG4)                                       \
  CBANG_FUNC_BASE(CLASS, RETURN, CALLBACK, CONST,                       \
                  CBANG_ARGS4(ARG1, ARG2, ARG3, ARG4))

#define CBANG_FUNCTOR_BASE5(CLASS, RETURN, CALLBACK, CONST, ARG1, ARG2, ARG3, \
                            ARG4, ARG5)                                 \
  CBANG_FUNC_BASE(CLASS, RETURN, CALLBACK, CONST,                       \
                  CBANG_ARGS5(ARG1, ARG2, ARG3, ARG4, ARG5))
