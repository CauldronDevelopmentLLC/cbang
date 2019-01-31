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

#include <cbang/macro/CPPMagic.h>

#include "FunctorBase.h"

#include <cbang/Exception.h>


#define CBANG_ARGS_OP(TYPE, NAME) TYPE NAME
#define CBANG_ARGS(...)                                         \
  CBANG_MAP_WITH_ID(CBANG_ARGS_OP, CBANG_COMMA, __VA_ARGS__)

#define CBANG_PARAMS_OP(TYPE, NAME) NAME
#define CBANG_PARAMS(...)                                       \
  CBANG_MAP_WITH_ID(CBANG_PARAMS_OP, CBANG_COMMA, __VA_ARGS__)


#define CBANG_FUNCTOR_(CLASS, BASE, RETURN, CALLBACK, CONST, ...)        \
  class CLASS : public BASE {                                           \
  public:                                                               \
    typedef RETURN (*func_t)(CBANG_ARGS(__VA_ARGS__));                  \
  protected:                                                            \
    func_t func;                                                        \
  public:                                                               \
    CLASS(func_t func) : func(func) {                                   \
      if (!func) CBANG_THROW("Functor cannot be NULL");                 \
    }                                                                   \
    RETURN CALLBACK(CBANG_ARGS(__VA_ARGS__)) CONST {                    \
      return (*func)(CBANG_PARAMS(__VA_ARGS__));                        \
    }                                                                   \
  }


#define CBANG_FUNCTOR(CLASS, BASE, RETURN, CALLBACK, ...)        \
  CBANG_FUNCTOR_(CLASS, BASE, RETURN, CALLBACK, , __VA_ARGS__)

#define CBANG_CONST_FUNCTOR(CLASS, BASE, RETURN, CALLBACK, ...) \
  CBANG_FUNCTOR_(CLASS, BASE, RETURN, CALLBACK, const, __VA_ARGS__)


namespace cb {
  namespace __cbang_functor_test {
    namespace base = __cbang_functor_base_test;

    CBANG_FUNCTOR(_0, base::_0, void, foo);
    CBANG_FUNCTOR(_1, base::_1, void, foo, int);
    CBANG_FUNCTOR(_2, base::_2, void, foo, int, int);

    CBANG_CONST_FUNCTOR(_c0, base::_0, void, foo);
    CBANG_CONST_FUNCTOR(_c1, base::_1, void, foo, int);
    CBANG_CONST_FUNCTOR(_c2, base::_2, void, foo, int, int);
  }
}
