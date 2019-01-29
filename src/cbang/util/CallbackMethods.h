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

#include "CallbackClasses.h"


#define CBANG_FCB0(CLASS, RETURN, METHOD)                               \
  RETURN METHOD(typename CLASS::func_t func) {                          \
    return METHOD(new CLASS(func));                                     \
  }


#define CBANG_FCB(CLASS, RETURN, METHOD, ARGS, PARAMS)                  \
  RETURN METHOD(ARGS, typename CLASS::func_t func) {                    \
    return METHOD(PARAMS, new CLASS(func));                             \
  }


#define CBANG_MFCB0(CLASS, RETURN, METHOD)                              \
  template <typename T>                                                 \
  RETURN METHOD(T *obj, typename CLASS<T>::member_t member) {           \
    return METHOD(new CLASS<T>(obj, member));                           \
  }


#define CBANG_MFCB(CLASS, RETURN, METHOD, ARGS, PARAMS)                 \
  template <typename T>                                                 \
  RETURN METHOD(ARGS, T *obj, typename CLASS<T>::member_t member) {     \
    return METHOD(PARAMS, new CLASS<T>(obj, member));                   \
  }


#if 199711L < __cplusplus
#define CBANG_FUCB0(...) CBANG_FCB0(__VA_ARGS__)
#define CBANG_FUCB(...) CBANG_FCB(__VA_ARGS__)
#else
#define CBANG_FUCB0(...)
#define CBANG_FUCB(...)
#endif


#define CBANG_CALLBACK_METHODS(CLASS, RETURN, METHOD)                   \
  CBANG_FCB0(CBANG_CONCAT(CLASS, Functor), RETURN, METHOD)              \
  CBANG_MFCB0(CBANG_CONCAT(CLASS, MemberFunctor), RETURN, METHOD)       \
  CBANG_FUCB0(CBANG_CONCAT(CLASS, Function), RETURN, METHOD)

#define CBANG_CALLBACK_METHODS1(CLASS, RETURN, METHOD, ARG1)            \
  CBANG_FCB(CBANG_CONCAT(CLASS, Functor), RETURN, METHOD,               \
            CBANG_ARGS1(ARG1), CBANG_PARAMS1())                         \
  CBANG_MFCB(CBANG_CONCAT(CLASS, MemberFunctor), RETURN, METHOD,        \
             CBANG_ARGS1(ARG1), CBANG_PARAMS1())                        \
  CBANG_FUCB(CBANG_CONCAT(CLASS, Function), RETURN, METHOD,             \
             CBANG_ARGS1(ARG1), CBANG_PARAMS1())

#define CBANG_CALLBACK_METHODS2(CLASS, RETURN, METHOD, ARG1, ARG2)      \
  CBANG_FCB(CBANG_CONCAT(CLASS, Functor), RETURN, METHOD,               \
            CBANG_ARGS2(ARG1, ARG2), CBANG_PARAMS2())                   \
  CBANG_MFCB(CBANG_CONCAT(CLASS, MemberFunctor), RETURN, METHOD,        \
             CBANG_ARGS2(ARG1, ARG2), CBANG_PARAMS2())                  \
  CBANG_FUCB(CBANG_CONCAT(CLASS, Function), RETURN, METHOD,             \
             CBANG_ARGS2(ARG1, ARG2), CBANG_PARAMS2())

#define CBANG_CALLBACK_METHODS3(CLASS, RETURN, METHOD, ARG1, ARG2, ARG3) \
  CBANG_FCB(CBANG_CONCAT(CLASS, Functor), RETURN, METHOD,               \
            CBANG_ARGS3(ARG1, ARG2, ARG3), CBANG_PARAMS3())             \
  CBANG_MFCB(CBANG_CONCAT(CLASS, MemberFunctor), RETURN, METHOD,        \
             CBANG_ARGS3(ARG1, ARG2, ARG3), CBANG_PARAMS3())            \
  CBANG_FUCB(CBANG_CONCAT(CLASS, Function), RETURN, METHOD,             \
             CBANG_ARGS3(ARG1, ARG2, ARG3), CBANG_PARAMS3())

#define CBANG_CALLBACK_METHODS4(CLASS, RETURN, METHOD, ARG1, ARG2, ARG3, \
                                ARG4)                                   \
  CBANG_FCB(CBANG_CONCAT(CLASS, Functor), RETURN, METHOD,               \
            CBANG_ARGS4(ARG1, ARG2, ARG3, ARG4), CBANG_PARAMS4())       \
  CBANG_MFCB(CBANG_CONCAT(CLASS, MemberFunctor), RETURN, METHOD,        \
             CBANG_ARGS4(ARG1, ARG2, ARG3, ARG4), CBANG_PARAMS4())      \
  CBANG_FUCB(CBANG_CONCAT(CLASS, Function), RETURN, METHOD,             \
             CBANG_ARGS4(ARG1, ARG2, ARG3, ARG4), CBANG_PARAMS4())

#define CBANG_CALLBACK_METHODS5(CLASS, RETURN, METHOD, ARG1, ARG2, ARG3, \
                                ARG4, ARG5)                             \
  CBANG_FCB(CBANG_CONCAT(CLASS, Functor), RETURN, METHOD,               \
            CBANG_ARGS5(ARG1, ARG2, ARG3, ARG4, ARG5), CBANG_PARAMS5()) \
  CBANG_MFCB(CBANG_CONCAT(CLASS, MemberFunctor), RETURN, METHOD,        \
             CBANG_ARGS5(ARG1, ARG2, ARG3, ARG4, ARG5), CBANG_PARAMS5()) \
  CBANG_FUCB(CBANG_CONCAT(CLASS, Function), RETURN, METHOD,             \
             CBANG_ARGS5(ARG1, ARG2, ARG3, ARG4, ARG5), CBANG_PARAMS5())


namespace cb {
  namespace __callback_classes_test__ {
    class __callback_methods_test__ {

      void foo(const SmartPointer<_0> &) {}
      void foo1(int, const SmartPointer<_0> &) {}
      void foo2(int, int, const SmartPointer<_0> &) {}
      void foo3(int, int, int, const SmartPointer<_0> &) {}
      void foo4(int, int, int, int, const SmartPointer<_0> &) {}
      void foo5(int, int, int, int, int, const SmartPointer<_0> &) {}

      CBANG_CALLBACK_METHODS(_0, void, foo);
      CBANG_CALLBACK_METHODS1(_0, void, foo1, int);
      CBANG_CALLBACK_METHODS2(_0, void, foo2, int, int);
      CBANG_CALLBACK_METHODS3(_0, void, foo3, int, int, int);
      CBANG_CALLBACK_METHODS4(_0, void, foo4, int, int, int, int);
      CBANG_CALLBACK_METHODS5(_0, void, foo5, int, int, int, int, int);
    };
  }
}
