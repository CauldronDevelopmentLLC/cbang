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


#define CBANG_CARGS_OP(TYPE, NAME) TYPE NAME,
#define CBANG_CARGS(...)                                        \
  CBANG_MAP_WITH_ID(CBANG_CARGS_OP, CBANG_EMPTY, __VA_ARGS__)

#define CBANG_CPARAMS_OP(TYPE, NAME) NAME,
#define CBANG_CPARAMS(...)                                      \
  CBANG_MAP_WITH_ID(CBANG_CPARAMS_OP, CBANG_EMPTY, __VA_ARGS__)


#define CBANG_FCB(CLASS, RETURN, METHOD, ...)                           \
  RETURN METHOD(CBANG_CARGS(__VA_ARGS__) CLASS::func_t func) {          \
    return METHOD(CBANG_CPARAMS(__VA_ARGS__) new CLASS(func));          \
  }


#define CBANG_MFCB(CLASS, RETURN, METHOD, ...)                          \
  template <typename T>                                                 \
  RETURN METHOD(CBANG_CARGS(__VA_ARGS__) T *obj,                        \
                typename CLASS<T>::member_t member) {                   \
    return METHOD(CBANG_CPARAMS(__VA_ARGS__) new CLASS<T>(obj, member)); \
  }


#if 199711L < __cplusplus
#define CBANG_FUCB(...) CBANG_FCB(__VA_ARGS__)
#else
#define CBANG_FUCB(...)
#endif


#define CBANG_CALLBACK_METHODS(CLASS, RETURN, METHOD, ...)              \
  CBANG_FCB(CBANG_CAT(CLASS, Functor), RETURN, METHOD, __VA_ARGS__)     \
  CBANG_MFCB(CBANG_CAT(CLASS, MemberFunctor), RETURN, METHOD, __VA_ARGS__) \
  CBANG_FUCB(CBANG_CAT(CLASS, Function), RETURN, METHOD, __VA_ARGS__)


namespace cb {
  namespace __callback_classes_test__ {
    class __callback_methods_test__ {

      void foo(const SmartPointer<_0> &) {}
      void foo1(int, const SmartPointer<_0> &) {}
      void foo2(int, int, const SmartPointer<_0> &) {}

      CBANG_CALLBACK_METHODS(_0, void, foo);
      CBANG_CALLBACK_METHODS(_0, void, foo1, int);
      CBANG_CALLBACK_METHODS(_0, void, foo2, int, int);
    };
  }
}
