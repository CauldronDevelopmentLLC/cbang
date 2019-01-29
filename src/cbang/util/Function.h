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

#if 199711L < __cplusplus

#include <functional>


#define CBANG_FUN(CLASS, BASE, RETURN, CALLBACK, CONST, ARGS, PARAMS)   \
  class CLASS : public BASE {                                           \
  public:                                                               \
    typedef std::function<RETURN (ARGS)> func_t;                        \
  protected:                                                            \
    func_t cb;                                                          \
  public:                                                               \
    CLASS(func_t cb) : cb(cb) {}                                        \
    RETURN CALLBACK(ARGS) CONST {return cb(PARAMS);}                    \
  }


#define CBANG_FUNCTION(CLASS, PARENT, RETURN, CALLBACK) \
  CBANG_FUN(CLASS, PARENT, RETURN, CALLBACK, , ,)

#define CBANG_FUNCTION1(CLASS, PARENT, RETURN, CALLBACK, ARG1)          \
  CBANG_FUN(CLASS, PARENT, RETURN, CALLBACK, , CBANG_ARGS1(ARG1), \
            CBANG_PARAMS1())

#define CBANG_FUNCTION2(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2)   \
  CBANG_FUN(CLASS, PARENT, RETURN, CALLBACK, , CBANG_ARGS2(ARG1, ARG2), \
            CBANG_PARAMS2())

#define CBANG_FUNCTION3(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2, ARG3) \
  CBANG_FUN(CLASS, PARENT, RETURN, CALLBACK, , CBANG_ARGS3(ARG1, ARG2, ARG3), \
            CBANG_PARAMS3())

#define CBANG_FUNCTION4(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2, ARG3, \
                        ARG4)                                           \
  CBANG_FUN(CLASS, PARENT, RETURN, CALLBACK, ,                          \
            CBANG_ARGS4(ARG1, ARG2, ARG3, ARG4), CBANG_PARAMS4())

#define CBANG_FUNCTION5(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2, ARG3, \
                        ARG4, ARG5)                                     \
  CBANG_FUN(CLASS, PARENT, RETURN, CALLBACK, ,                          \
            CBANG_ARGS5(ARG1, ARG2, ARG3, ARG4, ARG5), CBANG_PARAMS5())

#define CBANG_CONST_FUNCTION(CLASS, PARENT, RETURN, CALLBACK)   \
  CBANG_FUN(CLASS, PARENT, RETURN, CALLBACK, const, ,)

#define CBANG_CONST_FUNCTION1(CLASS, PARENT, RETURN, CALLBACK, ARG1)    \
  CBANG_FUN(CLASS, PARENT, RETURN, CALLBACK, const, CBANG_ARGS1(ARG1),  \
            CBANG_PARAMS1())

#define CBANG_CONST_FUNCTION2(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2)   \
  CBANG_FUN(CLASS, PARENT, RETURN, CALLBACK, const, CBANG_ARGS2(ARG1, ARG2), \
            CBANG_PARAMS2())

#define CBANG_CONST_FUNCTION3(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2, \
                              ARG3)                                     \
  CBANG_FUN(CLASS, PARENT, RETURN, CALLBACK, const,                     \
            CBANG_ARGS3(ARG1, ARG2, ARG3), CBANG_PARAMS3())

#define CBANG_CONST_FUNCTION4(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2, \
                              ARG3, ARG4)                               \
  CBANG_FUN(CLASS, PARENT, RETURN, CALLBACK, const,                     \
            CBANG_ARGS4(ARG1, ARG2, ARG3, ARG4), CBANG_PARAMS4())

#define CBANG_CONST_FUNCTION5(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2, \
                              ARG3, ARG4, ARG5)                         \
  CBANG_FUN(CLASS, PARENT, RETURN, CALLBACK, const,                     \
            CBANG_ARGS5(ARG1, ARG2, ARG3, ARG4, ARG5), CBANG_PARAMS5())

#else // 199711L < __cplusplus

#define CBANG_FUN(...)
#define CBANG_FUNCTION(...)
#define CBANG_FUNCTION1(...)
#define CBANG_FUNCTION2(...)
#define CBANG_FUNCTION3(...)
#define CBANG_FUNCTION4(...)
#define CBANG_FUNCTION5(...)

#define CBANG_CONST_FUNCTION(...)
#define CBANG_CONST_FUNCTION1(...)
#define CBANG_CONST_FUNCTION2(...)
#define CBANG_CONST_FUNCTION3(...)
#define CBANG_CONST_FUNCTION4(...)
#define CBANG_CONST_FUNCTION5(...)

#endif // 199711L < __cplusplus
